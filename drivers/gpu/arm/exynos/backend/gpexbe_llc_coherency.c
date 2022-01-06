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

#include <linux/io.h>
#include <linux/device.h>
#include <mali_kbase.h>
#include <gpex_utils.h>
#include <gpexbe_llc_coherency.h>

#include <soc/samsung/exynos-sci.h>

#define ACELITE 0x1
#define ACE 0x2

#define COHERENCY_MODE_NONE 0x1f
#define COHERENCY_MODE_IO 0x0
#define COHERENCY_MODE_FULL 0x1

#define AWUSER_VALUE 0x3ffffffff
#define ARUSER_VALUE 0xffffffff

#define AWUSER_HINT_MASK 0x3fffff
#define LLC_USER_REG4_WRITE_MASK 0xfffc00000
#define LLC_USER_REG4_READ_MASK 0x3fff

#define MALI_COHERENCY_ADDR 0x18420400
#define MALI_ARUSER_HINT_ADDR 0x18420420
#define MALI_AWUSER_HINT_ADDR 0x18420440
#define MALI_USER_REG4_ADDR 0x18420010

static struct _llc_coh_info {
	struct device **dev;
	void __iomem *g3d_coherency_features;
	void __iomem *aruser_hint;
	void __iomem *awuser_hint;
	void __iomem *user_reg4;

	int cur_llc_ways;
	spinlock_t llc_spinlock;
} llc_coh_info;

void gpexbe_llc_coherency_reg_map()
{
	llc_coh_info.g3d_coherency_features = ioremap(MALI_COHERENCY_ADDR, 4);
	llc_coh_info.aruser_hint = ioremap(MALI_ARUSER_HINT_ADDR, 4);
	llc_coh_info.awuser_hint = ioremap(MALI_AWUSER_HINT_ADDR, 4);
	llc_coh_info.user_reg4 = ioremap(MALI_USER_REG4_ADDR, 4);

	if (!llc_coh_info.g3d_coherency_features || !llc_coh_info.aruser_hint ||
	    !llc_coh_info.awuser_hint || !llc_coh_info.user_reg4) {
		dev_err(*llc_coh_info.dev,
			"Can't remap llc & coherency register: g3d_coherency_features(%p) \
				aruser_hint(%p) awuser_hint(%p) user_reg4(%p)\n",
			llc_coh_info.g3d_coherency_features, llc_coh_info.aruser_hint,
			llc_coh_info.awuser_hint, llc_coh_info.user_reg4);
	}
}

static void iounmap_if_valid_addr(void __iomem *addr)
{
	if (addr)
		iounmap(addr);
}

void gpexbe_llc_coherency_reg_unmap()
{
	iounmap_if_valid_addr(llc_coh_info.g3d_coherency_features);
	iounmap_if_valid_addr(llc_coh_info.aruser_hint);
	iounmap_if_valid_addr(llc_coh_info.awuser_hint);
	iounmap_if_valid_addr(llc_coh_info.user_reg4);
}

void gpexbe_llc_coherency_set_coherency_feature()
{
	if (llc_coh_info.g3d_coherency_features) {
		__raw_writel(ACELITE | ACE, llc_coh_info.g3d_coherency_features);
	}
}

void gpexbe_llc_coherency_set_aruser()
{
	__raw_writel(ARUSER_VALUE, llc_coh_info.aruser_hint);
}

void gpexbe_llc_coherency_set_awuser()
{
	__raw_writel(AWUSER_VALUE & AWUSER_HINT_MASK, llc_coh_info.awuser_hint);
	__raw_writel((AWUSER_VALUE & LLC_USER_REG4_WRITE_MASK) >> 22, llc_coh_info.user_reg4);
}

static ssize_t show_awuser_hint(char *buf)
{
	ssize_t ret = 0;
	void __iomem *reg;
	u64 val;

	reg = llc_coh_info.awuser_hint; /* SYSREG_G3D + GPU_AWUSER(0x0440)*/
	val = (__raw_readl(reg) & AWUSER_HINT_MASK);

	reg = llc_coh_info.user_reg4; /* SYSREG_G3D + USER_REG4(0x0010)*/
	val |= ((u64)(__raw_readl(reg) & LLC_USER_REG4_READ_MASK) << 22);

	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "0x%llx\n", val);

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_awuser_hint);

static ssize_t show_aruser_hint(char *buf)
{
	ssize_t ret = 0;
	void __iomem *reg;
	u64 val;

	reg = llc_coh_info.aruser_hint; /* SYSREG_G3D + GPU_ARUSER(0x0420)*/
	val = __raw_readl(reg);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "0x%llx\n", val);

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_aruser_hint);

static ssize_t show_coherency_mode(char *buf)
{
	ssize_t ret = 0;
	struct kbase_device *kbdev =
		(struct kbase_device *)container_of(llc_coh_info.dev, struct kbase_device, dev);
	u32 coh_mode;

	coh_mode = kbdev->system_coherency;

	if (coh_mode == COHERENCY_MODE_NONE)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "[CURRENT STATE] AXI(0x%llx, NO COHERENCY)\n", coh_mode);
	else if (coh_mode == COHERENCY_MODE_IO)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "[CURRENT STATE] "
				 "ACE-LITE(0x%llx, "
				 "IO COHERENCY)\n",
				 coh_mode);
	else if (coh_mode == COHERENCY_MODE_FULL)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "[CURRENT STATE] ACE and ACE-LITE(0x%llx, "
				 "FULL COHERENCY)\n",
				 coh_mode);
	else
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "[CURRENT STATE] Invalid value(0x%llx, NOT "
				 "0x1f, 0x0, 0x1)\n\n",
				 coh_mode);

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_coherency_mode);

int gpexbe_llc_coherency_init(struct device **dev)
{
	llc_coh_info.dev = dev;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(aruser_hint, show_aruser_hint);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(awuser_hint, show_awuser_hint);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(coherency_mode, show_coherency_mode);

	spin_lock_init(&llc_coh_info.llc_spinlock);
	llc_coh_info.cur_llc_ways = 0;

	return 0;
}

void gpexbe_llc_coherency_term()
{
	gpexbe_llc_coherency_set_ways(0);

	llc_coh_info.dev = NULL;
	llc_coh_info.cur_llc_ways = 0;
}

int gpexbe_llc_coherency_set_ways(int ways)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&llc_coh_info.llc_spinlock, flags);

	/* Do not set GPU LLC again if GPU_LLC is already set to desired value */
	if (ways == llc_coh_info.cur_llc_ways) {
		spin_unlock_irqrestore(&llc_coh_info.llc_spinlock, flags);
		return ret;
	}

	/* Dealloc GPU LLC if not needed anymore
	 * Also it need to be deallocated before allocating different amount
	 * e.g: 0 -> 10 -> 16 -> 0  BAD!!
	 *      0 -> 10 -> 0  -> 16 GOOD!!
	 */
	if (ways == 0 || llc_coh_info.cur_llc_ways > 0) {
		ret = llc_region_alloc(LLC_REGION_GPU, 0, 0);

		if (ret)
			goto llc_set_finish;
	}

	if (ways > 0)
		ret = llc_region_alloc(LLC_REGION_GPU, 1, ways);

llc_set_finish:
	spin_unlock_irqrestore(&llc_coh_info.llc_spinlock, flags);

	if (ret)
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: failed to allocate llc: errno(%d)\n", __func__,
			ret);
	else {
		llc_coh_info.cur_llc_ways = ways;
		GPU_LOG(MALI_EXYNOS_DEBUG, "%s: llc set with way(%d) (1 way == 512 KB)\n", __func__,
			ways);
	}

	return ret;
}
