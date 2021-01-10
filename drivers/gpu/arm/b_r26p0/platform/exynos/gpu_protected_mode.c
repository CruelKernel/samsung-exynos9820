/*
 *
 * (C) COPYRIGHT 2020 Samsung Electronics Inc. All rights reserved.
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
#include <mali_malisw.h>
#include <mali_kbase_hwaccess_jm.h>
#include <mali_kbase_device_internal.h>
/*#include <soc/samsung/exynos-smc.h*/
#include <linux/smc.h>

/* SMC CALL return value for Successfully works */
#define GPU_SMC_TZPC_OK 0

int exynos_secure_mode_enable(struct protected_mode_device *pdev)
{
	/* enable secure mode : TZPC */
	struct kbase_device *kbdev = pdev->data;
	struct exynos_context *platform;
	unsigned long flags;
	int ret = 0;

	if (!kbdev) {
		ret = -EINVAL;
		goto secure_out;
	}

	platform = (struct exynos_context *) kbdev->platform_context;

	spin_lock_irqsave(&platform->exynos_smc_lock, flags);
	if (platform->exynos_smc_enabled) {
		spin_unlock_irqrestore(&platform->exynos_smc_lock, flags);
		goto secure_out;
	}

	ret = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_ENABLE);

	if (ret == GPU_SMC_TZPC_OK) {
		platform->exynos_smc_enabled = true;
	}
	spin_unlock_irqrestore(&platform->exynos_smc_lock, flags);

	GPU_LOG(DVFS_INFO, LSI_SECURE_WORLD_ENTER, 0u, 0u, "LSI_SECURE_WORLD_ENTER\n");

	if (ret == GPU_SMC_TZPC_OK) {
		ret = 0;
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: Enter Secure World by GPU\n", __func__);
	} else {
		GPU_LOG(DVFS_ERROR, LSI_GPU_SECURE, 0u, 0u, "%s: failed exynos_smc() ret : %d\n", __func__, ret);
	}

secure_out:
	return ret;
}

int exynos_secure_mode_disable(struct protected_mode_device *pdev)
{
	/* Turn off secure mode and reset GPU : TZPC */
	struct kbase_device *kbdev = pdev->data;
	struct exynos_context *platform;
	unsigned long flags;
	int ret = 0;

	if (!kbdev) {
		ret = -EINVAL;
		goto secure_out;
	}

	platform = (struct exynos_context *) kbdev->platform_context;

	spin_lock_irqsave(&platform->exynos_smc_lock, flags);
	if (!platform->exynos_smc_enabled) {
		spin_unlock_irqrestore(&platform->exynos_smc_lock, flags);
		goto secure_out;
	}

	ret = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_DISABLE);

	if (ret == GPU_SMC_TZPC_OK) {
		platform->exynos_smc_enabled = false;
	}
	spin_unlock_irqrestore(&platform->exynos_smc_lock, flags);

	GPU_LOG(DVFS_INFO, LSI_SECURE_WORLD_EXIT, 0u, 0u, "LSI_SECURE_WORLD_EXIT\n");

	if (ret == GPU_SMC_TZPC_OK) {
		ret = 0;
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: Exit Secure World by GPU\n", __func__);
	} else {
		GPU_LOG(DVFS_ERROR, LSI_GPU_SECURE, 0u, 0u, "%s: failed exynos_smc() ret : %d\n", __func__, ret);
	}

secure_out:
	return ret;
}

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY)
struct protected_mode_ops exynos_protected_ops = {
	.protected_mode_enable = exynos_secure_mode_enable,
	.protected_mode_disable = exynos_secure_mode_disable
};

static void kbasep_js_cacheclean(struct kbase_device *kbdev)
{
	/* Limit the number of loops to avoid a hang if the interrupt is missed */
	u32 max_loops = KBASE_CLEAN_CACHE_MAX_LOOPS;

	GPU_LOG(DVFS_INFO, LSI_SECURE_CACHE, 0u, 0u, "GPU CACHE WORKING for Secure Rendering\n");
	/* use GPU_COMMAND completion solution */
	/* clean the caches */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_CLEAN_CACHES);

	/* wait for cache flush to complete before continuing */
	while (--max_loops && (kbase_reg_read(kbdev,
		GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)) & CLEAN_CACHES_COMPLETED) == 0)
		;

	/* clear the CLEAN_CACHES_COMPLETED irq */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), CLEAN_CACHES_COMPLETED);
	GPU_LOG(DVFS_INFO, LSI_SECURE_CACHE_END, 0u, 0u, "GPU CACHE WORKING for Secure Rendering\n");
}

int kbase_jm_enter_protected_mode_exynos(struct kbase_device *kbdev,
		struct kbase_jd_atom **katom, int idx, int js)
{
	int err = 0;

	CSTD_UNUSED(katom);
	CSTD_UNUSED(idx);
	CSTD_UNUSED(js);

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		/* Switch GPU to protected mode */
		kbasep_js_cacheclean(kbdev);
		err = exynos_secure_mode_enable(kbdev->protected_dev);

		if (err)
			dev_warn(kbdev->dev, "Failed to enable protected mode: %d\n", err);
		else
		kbdev->protected_mode = true;
	}

	return 0;
}

int kbase_jm_exit_protected_mode_exynos(struct kbase_device *kbdev,
		struct kbase_jd_atom **katom, int idx, int js)
{
	int err = 0;

	CSTD_UNUSED(katom);
	CSTD_UNUSED(idx);
	CSTD_UNUSED(js);

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		/* Switch GPU to protected mode */
		kbasep_js_cacheclean(kbdev);
		err = exynos_secure_mode_disable(kbdev->protected_dev);

		if (err)
			dev_warn(kbdev->dev, "Failed to enable protected mode: %d\n",
					err);
		else
			kbdev->protected_mode = false;
	}

	return 0;
}

int kbase_protected_mode_disable_exynos(struct kbase_device *kbdev)
{
	int err = 0;

	if (!kbdev)
		return -EINVAL;

	if (kbdev->protected_mode == true) {

		/* Switch GPU to non-secure mode */
		err = exynos_secure_mode_disable(kbdev->protected_dev);
		if (err)
			dev_warn(kbdev->dev, "Failed to disable secure mode: %d\n", err);
		else
			kbdev->protected_mode = false;
	}

	return err;
}
#elif IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_ARM)
/*
 * Call back functions for PROTECTED_CALLBACKS
 */
static int exynos_secure_mode_enable_arm(struct protected_mode_device *pdev)
{
	int ret = 0;
	struct kbase_device *kbdev = pdev->data;

	if (!kbdev)
		return 0;

	ret = kbase_pm_protected_mode_enable(kbdev);
	if (ret != 0)
		return ret;

	return exynos_secure_mode_enable(pdev);
}

static int exynos_secure_mode_disable_arm(struct protected_mode_device *pdev)
{
	int ret = 0;
	struct kbase_device *kbdev = pdev->data;

	if (!kbdev)
		return 0;

	ret = kbase_pm_protected_mode_disable(kbdev);
	if (ret != 0)
		return ret;

	return exynos_secure_mode_disable(pdev);
}

struct protected_mode_ops exynos_protected_ops_arm = {
	.protected_mode_enable = exynos_secure_mode_enable_arm,
	.protected_mode_disable = exynos_secure_mode_disable_arm
};
#endif /* CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY */
