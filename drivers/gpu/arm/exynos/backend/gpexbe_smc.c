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
#include <gpexbe_smc.h>

/* Uses */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <linux/smc.h>
#else
#include <soc/samsung/exynos-smc.h>
#endif

#include <gpex_utils.h>

struct _smc_info {
	bool protection_enabled;
	spinlock_t lock;
};

static struct _smc_info smc_info;

int gpexbe_smc_protection_enable()
{
	int err;
	unsigned long flags;

	spin_lock_irqsave(&smc_info.lock, flags);
	if (smc_info.protection_enabled) {
		spin_unlock_irqrestore(&smc_info.lock, flags);
		return 0;
	}

	err = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_ENABLE);

	if (!err)
		smc_info.protection_enabled = true;

	spin_unlock_irqrestore(&smc_info.lock, flags);

	if (!err)
		GPU_LOG(MALI_EXYNOS_INFO, "%s: Enter Secure World by GPU\n", __func__);
	else
		GPU_LOG_DETAILED(MALI_EXYNOS_ERROR, LSI_GPU_SECURE, 0u, 0u,
				 "%s: failed to enter secure world ret : %d\n", __func__, err);

	return err;
}

int gpexbe_smc_protection_disable()
{
	int err;
	unsigned long flags;

	spin_lock_irqsave(&smc_info.lock, flags);
	if (!smc_info.protection_enabled) {
		spin_unlock_irqrestore(&smc_info.lock, flags);
		return 0;
	}

	err = exynos_smc(SMC_PROTECTION_SET, 0, PROT_G3D, SMC_PROTECTION_DISABLE);

	if (!err)
		smc_info.protection_enabled = false;

	spin_unlock_irqrestore(&smc_info.lock, flags);

	if (!err)
		GPU_LOG(MALI_EXYNOS_INFO, "%s: Exit Secure World by GPU\n", __func__);
	else
		GPU_LOG_DETAILED(MALI_EXYNOS_ERROR, LSI_GPU_SECURE, 0u, 0u,
				 "%s: failed to exit secure world ret : %d\n", __func__, err);

	return err;
}

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_SMC_NOTIFY_GPU)

#ifndef SMC_DRM_G3D_POWER_ON
/* Older kernel versions have SMC_DRM_G3D_POWER_ON as SMC_DRM_G3D_PPCFW_RESTORE
 * but they share same value
 */
#define SMC_DRM_G3D_POWER_ON SMC_DRM_G3D_PPCFW_RESTORE
#endif

void gpexbe_smc_notify_power_on()
{
	exynos_smc(SMC_DRM_G3D_POWER_ON, 0, 0, 0);
}

void gpexbe_smc_notify_power_off()
{
	exynos_smc(SMC_DRM_G3D_POWER_OFF, 0, 0, 0);
}
#else
void gpexbe_smc_notify_power_on()
{
}
void gpexbe_smc_notify_power_off()
{
}
#endif

int gpexbe_smc_init(void)
{
	spin_lock_init(&smc_info.lock);
	smc_info.protection_enabled = false;

	return 0;
}

void gpexbe_smc_term(void)
{
	gpexbe_smc_protection_disable();
	smc_info.protection_enabled = false;
}
