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
#include "mali_exynos_kbase_entrypoint.h"

/* Uses */
#include <mali_kbase.h>

#include <gpex_common.h>
#include <gpex_platform.h>
#include <gpex_utils.h>
#include <gpex_pm.h>
#include <gpex_qos.h>
#include <gpex_clock.h>
#include <gpex_gts.h>
#include <gpex_tsg.h>
#include <gpex_cmar_boost.h>

#include <gpexbe_llc_coherency.h>
#include <gpexbe_utilization.h>
#include <gpexbe_pm.h>
#include <gpexbe_power_cycle_wa.h>
#include <gpexbe_secure.h>
#include <gpexbe_dmabuf.h>

#include <mali_exynos_ioctl.h>

static int mali_exynos_ioctl_amigo_flags_fn(struct kbase_context *kctx,
					    struct mali_exynos_ioctl_amigo_flags *flags)
{
	flags->flags = (uint32_t)(gpex_tsg_get_amigo_flags());
	return 0;
}

static int mali_exynos_ioctl_read_gts_info_fn(struct kbase_context *kctx,
					      struct mali_exynos_ioctl_gts_info *info)
{
	return gpex_gts_get_ioctl_gts_info(info);
}

static int mali_exynos_ioctl_hcm_pmqos_fn(struct kbase_context *kctx,
					  struct mali_exynos_ioctl_hcm_pmqos *pmqos)
{
	if (gpex_gts_get_hcm_mode() != pmqos->mode) {
		gpex_gts_set_hcm_mode(pmqos->mode);
		gpex_qos_set_from_clock(gpex_clock_get_cur_clock());
	}

	return 0;
}

static int mali_exynos_ioctl_cmar_boost_fn(struct kbase_context *kctx,
					   struct mali_exynos_ioctl_cmar_boost *cb_flag)
{
	if (!cb_flag)
		return -EINVAL;

	return gpex_cmar_boost_set_flag(kctx->platform_data, cb_flag->flags);
}

static int kbase_api_singlebuffer_boost(struct kbase_context *kctx,
					struct kbase_ioctl_slsi_singlebuffer_boost_flags *flags)
{
	if (!flags)
		return -EINVAL;

/* These are defined as UK_FUNC_ID + 49, 50 in legacy DDK, and UK_FUNC_ID is 50 */
#define KBASE_FUNC_SET_SINGLEBUFFER_BOOST (561)
#define KBASE_FUNC_UNSET_SINGLEBUFFER_BOOST (562)

	switch (flags->flags) {
	case KBASE_FUNC_SET_SINGLEBUFFER_BOOST:
		return gpex_cmar_boost_set_flag(kctx->platform_data, CMAR_BOOST_SET_RT);

	case KBASE_FUNC_UNSET_SINGLEBUFFER_BOOST:
		return gpex_cmar_boost_set_flag(kctx->platform_data, CMAR_BOOST_SET_DEFAULT);

	default:
		break;
	}

	return -EINVAL;
}

#define KBASE_HANDLE_IOCTL_IN(cmd, function, type, arg)                                            \
	do {                                                                                       \
		type param;                                                                        \
		int ret, err;                                                                      \
		dev_dbg(arg->kbdev->dev, "Enter ioctl %s\n", #function);                           \
		err = copy_from_user(&param, uarg, sizeof(param));                                 \
		if (err)                                                                           \
			return -EFAULT;                                                            \
		ret = function(arg, &param);                                                       \
		dev_dbg(arg->kbdev->dev, "Return %d from ioctl %s\n", ret, #function);             \
		return ret;                                                                        \
	} while (0)

#define KBASE_HANDLE_IOCTL_OUT(cmd, function, type, arg)                                           \
	do {                                                                                       \
		type param;                                                                        \
		int ret, err;                                                                      \
		memset(&param, 0, sizeof(param));                                                  \
		ret = function(arg, &param);                                                       \
		err = copy_to_user(uarg, &param, sizeof(param));                                   \
		if (err)                                                                           \
			return -EFAULT;                                                            \
		dev_dbg(arg->kbdev->dev, "Return %d from ioctl %s\n", ret, #function);             \
		return ret;                                                                        \
	} while (0)

int mali_exynos_ioctl(struct kbase_context *kctx, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case MALI_EXYNOS_IOCTL_AMIGO_FLAGS:
		KBASE_HANDLE_IOCTL_OUT(cmd, mali_exynos_ioctl_amigo_flags_fn,
				       struct mali_exynos_ioctl_amigo_flags, kctx);
		break;

	case MALI_EXYNOS_IOCTL_READ_GTS_INFO:
		KBASE_HANDLE_IOCTL_OUT(cmd, mali_exynos_ioctl_read_gts_info_fn,
				       struct mali_exynos_ioctl_gts_info, kctx);
		break;

	case MALI_EXYNOS_IOCTL_HCM_PMQOS:
		KBASE_HANDLE_IOCTL_IN(cmd, mali_exynos_ioctl_hcm_pmqos_fn,
				      struct mali_exynos_ioctl_hcm_pmqos, kctx);
		break;

	case MALI_EXYNOS_IOCTL_MEM_USAGE_ADD:
		/* Deprecated IOCTL
		 * Before, gpu mem usage was sent to the kernel from userspace via this IOCTL.
		 * Now gpu mem usage is caculated directely from the kernel driver,
		 * so there's no need for this IOCTL.
		 * */
		break;

	case MALI_EXYNOS_IOCTL_CMAR_BOOST:
		KBASE_HANDLE_IOCTL_IN(cmd, mali_exynos_ioctl_cmar_boost_fn,
				      struct mali_exynos_ioctl_cmar_boost, kctx);
		break;

	case KBASE_IOCTL_SLSI_SINGLEBUFFER_BOOST_FLAGS:
		/* Deprecated IOCTL only used by legacy UMD */
		KBASE_HANDLE_IOCTL_IN(cmd, kbase_api_singlebuffer_boost,
				      struct kbase_ioctl_slsi_singlebuffer_boost_flags, kctx);
		break;
	}

	return 0;
}

void mali_exynos_set_thread_priority(struct kbase_context *kctx)
{
	gpex_cmar_boost_set_thread_priority(kctx->platform_data);
}

void mali_exynos_coherency_reg_map()
{
	gpexbe_llc_coherency_reg_map();
}

void mali_exynos_coherency_reg_unmap()
{
	gpexbe_llc_coherency_reg_unmap();
}

void mali_exynos_coherency_set_coherency_feature()
{
	gpexbe_llc_coherency_set_coherency_feature();
}

void mali_exynos_llc_set_aruser()
{
	gpexbe_llc_coherency_set_aruser();
}

void mali_exynos_llc_set_awuser()
{
	gpexbe_llc_coherency_set_awuser();
}

int mali_exynos_set_pm_state_resume_begin(void)
{
	return gpex_pm_set_state(GPEX_PM_STATE_RESUME_BEGIN);
}

int mali_exynos_set_pm_state_resume_end(void)
{
	return gpex_pm_set_state(GPEX_PM_STATE_RESUME_END);
}

void mali_exynos_set_jobslot_status(int slot, bool is_active)
{
	if (slot == 0)
		gpex_gts_set_jobslot_status(is_active);
}

void mali_exynos_update_jobslot_util(int slot, bool gpu_active, u32 ns_time)
{
	if (slot == 0)
		gpex_gts_update_jobslot_util(gpu_active, ns_time);
}

void mali_exynos_update_job_load(struct kbase_jd_atom *katom, ktime_t *end_timestamp)
{
	if (katom && end_timestamp)
		gpexbe_utilization_update_job_load(katom, end_timestamp);
}

int mali_exynos_set_count(struct kbase_jd_atom *katom, u32 status, bool stop)
{
	/* Return whether katom will run on the GPU or not. Currently only soft jobs and
	 * dependency-only atoms do not run on the GPU */
	if (((katom->core_req & BASE_JD_REQ_SOFT_JOB) ||
	     ((katom->core_req & BASE_JD_REQ_ATOM_TYPE) == BASE_JD_REQ_DEP)) ||
	    ((status == KBASE_JD_ATOM_STATE_COMPLETED)))
		return 0;

	switch (status) {
	case KBASE_JD_ATOM_STATE_UNUSED:
		status = GPEX_TSG_ATOM_STATE_UNUSED;
		break;
	case KBASE_JD_ATOM_STATE_QUEUED:
		status = GPEX_TSG_ATOM_STATE_QUEUED;
		break;
	case KBASE_JD_ATOM_STATE_IN_JS:
		status = GPEX_TSG_ATOM_STATE_IN_JS;
		break;
	case KBASE_JD_ATOM_STATE_HW_COMPLETED:
		status = GPEX_TSG_ATOM_STATE_HW_COMPLETED;
		break;
	}

	return gpex_tsg_set_count(status, stop);
}

int mali_exynos_get_gpu_power_state(void)
{
	return gpexbe_pm_get_status();
}

static int gpu_power_on(struct kbase_device *kbdev)
{
	int ret = 0;
	ret = gpex_pm_power_on(kbdev->dev);

	if (ret == 0) {
		/* GPU state was lost. must return 1 for mali driver */
		return 1;
	} else if (ret > 0) {
		/* GPU runtime PM status was already active, so GPU state is not lost */
		return 0;
	} else {
		/* Major error.... gpu not powering on? */
		/* TODO: print some dire warning here */
		return 0;
	}
}

static void gpu_power_off(struct kbase_device *kbdev)
{
	gpex_pm_power_autosuspend(kbdev->dev);
}

static void gpu_power_suspend(struct kbase_device *kbdev)
{
	gpex_pm_suspend(kbdev->dev);
}

static int gpu_device_runtime_init(struct kbase_device *kbdev)
{
	return gpex_pm_runtime_init(kbdev->dev);
}

static void gpu_device_runtime_disable(struct kbase_device *kbdev)
{
	gpex_pm_runtime_term(kbdev->dev);
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	gpex_pm_runtime_off_prepare(kbdev->dev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	return gpex_pm_runtime_on_prepare(kbdev->dev);
}

/* Secure Rendering functions Start */
int mali_exynos_legacy_jm_enter_protected_mode(struct kbase_device *kbdev)
{
	return gpexbe_secure_legacy_jm_enter_protected_mode(kbdev);
}

int mali_exynos_legacy_jm_exit_protected_mode(struct kbase_device *kbdev)
{
	return gpexbe_secure_legacy_jm_exit_protected_mode(kbdev);
}

int mali_exynos_legacy_pm_exit_protected_mode(struct kbase_device *kbdev)
{
	return gpexbe_secure_legacy_pm_exit_protected_mode(kbdev);
}

struct protected_mode_ops *mali_exynos_get_protected_ops(void)
{
	return gpexbe_secure_get_protected_mode_ops();
}
/* Secure Rendering functions End */

void mali_exynos_sysfs_set_gpu_model_callback(sysfs_read_func show_gpu_model_fn)
{
	gpex_utils_sysfs_set_gpu_model_callback(show_gpu_model_fn);
}

bool mali_exynos_dmabuf_is_cached(struct dma_buf *dmabuf)
{
	return gpexbe_dmabuf_is_cached(dmabuf);
}

static int mali_exynos_kbase_entrypoint_init(struct kbase_device *kbdev)
{
	kbdev->platform_context = (void *)gpex_platform_init(&kbdev->dev);

	return 0;
}

static void mali_exynos_kbase_entrypoint_term(struct kbase_device *kbdev)
{
	gpex_platform_term();
	kbdev->platform_context = NULL;
}

static int mali_exynos_kbase_context_init(struct kbase_context *kctx)
{
	struct platform_context *pctx = kcalloc(1, sizeof(struct platform_context), GFP_KERNEL);

	pctx->cmar_boost = CMAR_BOOST_DEFAULT;
	pctx->pid = kctx->pid;

	kctx->platform_data = pctx;

	return 0;
}

static void mali_exynos_kbase_context_term(struct kbase_context *kctx)
{
	kfree(kctx->platform_data);
}

struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = &mali_exynos_kbase_entrypoint_init,
	.platform_term_func = &mali_exynos_kbase_entrypoint_term,
	.platform_handler_context_init_func = &mali_exynos_kbase_context_init,
	.platform_handler_context_term_func = &mali_exynos_kbase_context_term,
	.platform_late_init_func = NULL,
	.platform_late_term_func = NULL,
	.platform_handler_atom_submit_func = NULL,
	.platform_handler_atom_complete_func = NULL
};

struct kbase_pm_callback_conf pm_callbacks = {
	.power_suspend_callback = gpu_power_suspend,
	.power_on_callback = gpu_power_on,
	.power_off_callback = gpu_power_off,
	.power_runtime_init_callback = gpu_device_runtime_init,
	.power_runtime_term_callback = gpu_device_runtime_disable,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_off_callback = pm_callback_runtime_off,
};

MODULE_SOFTDEP("pre: exynos-acme");
