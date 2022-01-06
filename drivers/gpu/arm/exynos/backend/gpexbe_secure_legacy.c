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
#include <gpexbe_secure.h>

/* Uses */
#include <linux/protected_mode_switcher.h>

#include <mali_kbase.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase_hwaccess_jm.h>

#include <gpexbe_smc.h>
#include <gpex_utils.h>

static int exynos_secure_mode_enable(struct protected_mode_device *pdev)
{
	CSTD_UNUSED(pdev);

	return gpexbe_smc_protection_enable();
}

static int exynos_secure_mode_disable(struct protected_mode_device *pdev)
{
	struct kbase_device *kbdev = pdev->data;

	if (kbdev->protected_mode)
		return gpexbe_smc_protection_disable();
	else
		return kbase_pm_protected_mode_disable(kbdev);
}

struct protected_mode_ops *gpexbe_secure_get_protected_mode_ops()
{
	static struct protected_mode_ops exynos_protected_ops = {
		.protected_mode_enable = &exynos_secure_mode_enable,
		.protected_mode_disable = &exynos_secure_mode_disable
	};

	return &exynos_protected_ops;
}

static void kbasep_js_cacheclean(struct kbase_device *kbdev)
{
	/* Limit the number of loops to avoid a hang if the interrupt is missed */
	u32 max_loops = KBASE_CLEAN_CACHE_MAX_LOOPS;

	/* use GPU_COMMAND completion solution */
	/* clean the caches */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_CLEAN_CACHES);

	/* wait for cache flush to complete before continuing */
	while (--max_loops && (kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)) &
			       CLEAN_CACHES_COMPLETED) == 0)
		;

	/* clear the CLEAN_CACHES_COMPLETED irq */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), CLEAN_CACHES_COMPLETED);
}

int gpexbe_secure_legacy_jm_enter_protected_mode(struct kbase_device *kbdev)
{
	if (!kbdev)
		return -EINVAL;

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		int err = 0;

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

int gpexbe_secure_legacy_jm_exit_protected_mode(struct kbase_device *kbdev)
{
	if (!kbdev)
		return -EINVAL;

	if (!kbdev->protected_mode)
		return kbase_pm_protected_mode_disable(kbdev);

	if (kbase_gpu_atoms_submitted_any(kbdev))
		return -EAGAIN;

	if (kbdev->protected_ops) {
		int err = 0;

		/* Switch GPU to protected mode */
		kbasep_js_cacheclean(kbdev);
		err = exynos_secure_mode_disable(kbdev->protected_dev);

		if (err)
			dev_warn(kbdev->dev, "Failed to disable protected mode: %d\n", err);
		else
			kbdev->protected_mode = false;
	}

	return 0;
}

int gpexbe_secure_legacy_pm_exit_protected_mode(struct kbase_device *kbdev)
{
	int err = 0;

	if (!kbdev)
		return -EINVAL;

	if (kbdev->protected_mode == true) {
		/* Switch GPU to non-secure mode */
		err = exynos_secure_mode_disable(kbdev->protected_dev);
		if (err)
			dev_warn(kbdev->dev, "Failed to disable protected mode: %d\n", err);
		else
			kbdev->protected_mode = false;
	}

	return err;
}
