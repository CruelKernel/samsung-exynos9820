#include "../pmucal_common.h"
#include "../pmucal_cpu.h"
#include "../pmucal_local.h"
#include "../pmucal_rae.h"
#include "../pmucal_system.h"
#include "../pmucal_powermode.h"

#include "flexpmu_cal_cpu_exynos9820.h"
#include "flexpmu_cal_local_exynos9820.h"
#include "flexpmu_cal_p2vmap_exynos9820.h"
#include "flexpmu_cal_system_exynos9820.h"
#include "flexpmu_cal_powermode_exynos9820.h"
#include "flexpmu_cal_define_exynos9820.h"

#ifdef CONFIG_CP_PMUCAL 
#include "../pmucal_cp.h"
#include "pmucal_cp_exynos9820.h"
#endif

#include "cmucal-node.c"
#include "cmucal-qch.c"
#include "cmucal-sfr.c"
#include "cmucal-vclk.c"
#include "cmucal-vclklut.c"

#include "clkout_exynos9820.c"

#include "acpm_dvfs_exynos9820.h"

#include "asv_exynos9820.h"

#include "../ra.h"

#if defined(CONFIG_SEC_DEBUG)
#include <soc/samsung/exynos-pm.h>
#endif

#include <soc/samsung/cmu_ewf.h>

void __iomem *cmu_mmc;
void __iomem *cmu_busc;

#define PLL_CON0_PLL_MMC	(0x160)
#define PLL_CON1_PLL_MMC	(0x164)
#define PLL_CON2_PLL_MMC	(0x168)
#define PLL_CON3_PLL_MMC	(0x16c)

#define PLL_ENABLE_SHIFT	(31)

#define MANUAL_MODE		(0x2)

#define MFR_MASK		(0xff)
#define MRR_MASK		(0x3f)
#define MFR_SHIFT		(16)
#define MRR_SHIFT		(24)
#define SSCG_EN			(16)

static int cmu_stable_done(void __iomem *cmu,
			unsigned char shift,
			unsigned int done,
			int usec)
{
	unsigned int result;

	do {
		result = get_bit(cmu, shift);

		if (result == done)
			return 0;
		udelay(1);
	} while (--usec > 0);

	return -EVCLKTIMEOUT;
}

int pll_mmc_enable(int enable)
{
	unsigned int reg;
	unsigned int cmu_mode;
	int ret;

	if (!cmu_mmc) {
		pr_err("%s: cmu_mmc cmuioremap failed\n", __func__);
		return -1;
	}

	cmu_mode = readl(cmu_mmc + PLL_CON2_PLL_MMC);
	writel(MANUAL_MODE, cmu_mmc + PLL_CON2_PLL_MMC);

	reg = readl(cmu_mmc + PLL_CON0_PLL_MMC);

	if (!enable) {
		reg &= ~(PLL_MUX_SEL);
		writel(reg, cmu_mmc + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_mmc + PLL_CON0_PLL_MMC, PLL_MUX_BUSY_SHIFT, 0, 100);

		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	if (enable)
		reg |= 1 << PLL_ENABLE_SHIFT;
	else
		reg &= ~(1 << PLL_ENABLE_SHIFT);

	writel(reg, cmu_mmc + PLL_CON0_PLL_MMC);

	if (enable) {
		ret = cmu_stable_done(cmu_mmc + PLL_CON0_PLL_MMC, PLL_STABLE_SHIFT, 1, 100);
		if (ret)
			pr_err("pll time out, \'PLL_MMC\' %d\n", enable);

		reg |= PLL_MUX_SEL;
		writel(reg, cmu_mmc + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_mmc + PLL_CON0_PLL_MMC, PLL_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	writel(cmu_mode, cmu_mmc + PLL_CON2_PLL_MMC);

	return ret;
}

int cal_pll_mmc_check(void)
{
       unsigned int reg;
       bool ret = false;

       reg = readl(cmu_mmc + PLL_CON1_PLL_MMC);

       if (reg & (1 << SSCG_EN))
               ret = true;

       return ret;
}

int cal_pll_mmc_set_ssc(unsigned int mfr, unsigned int mrr, unsigned int ssc_on)
{
	unsigned int reg;
	int ret = 0;

	ret = pll_mmc_enable(0);
	if (ret) {
		pr_err("%s: pll_mmc_disable failed\n", __func__);
		return ret;
	}

	reg = readl(cmu_mmc + PLL_CON3_PLL_MMC);
	reg &= ~((MFR_MASK << MFR_SHIFT) | (MRR_MASK << MRR_SHIFT));

	if (ssc_on)
		reg |= ((mfr & MFR_MASK) << MFR_SHIFT) | ((mrr & MRR_MASK) << MRR_SHIFT);

	writel(reg, cmu_mmc + PLL_CON3_PLL_MMC);

	reg = readl(cmu_mmc + PLL_CON1_PLL_MMC);

	if (ssc_on)
		reg |= 1 << SSCG_EN;
	else
		reg &= ~(1 << SSCG_EN);

	writel(reg, cmu_mmc + PLL_CON1_PLL_MMC);
	ret = pll_mmc_enable(1);
	if (ret)
		pr_err("%s: pll_mmc_enable failed\n", __func__);

	return ret;
}

#define EXYNOS9820_CMU_MMC_BASE		(0x18C00000)
#define EXYNOS9820_CMU_BUSC_BASE	(0x1A200000)

#define QCH_CON_TREX_D0_BUSC_QCH_OFFSET	(0x319c)
#define IGNORE_FORCE_PM_EN		(2)

void exynos9820_cal_data_init(void)
{
	pr_info("%s: cal data init\n", __func__);

	/* cpu inform sfr initialize */
	pmucal_sys_powermode[SYS_SICD] = CPU_INFORM_SICD;
	pmucal_sys_powermode[SYS_SLEEP] = CPU_INFORM_SLEEP;
	pmucal_sys_powermode[SYS_SLEEP_USBL2] = CPU_INFORM_SLEEP_USBL2;

	cpu_inform_c2 = CPU_INFORM_C2;
	cpu_inform_cpd = CPU_INFORM_CPD;

	cmu_mmc = ioremap(EXYNOS9820_CMU_MMC_BASE, SZ_4K);
	if (!cmu_mmc)
		pr_err("%s: cmu_mmc ioremap failed\n", __func__);

	cmu_busc = ioremap(EXYNOS9820_CMU_BUSC_BASE, SZ_16K);
	if (!cmu_busc)
		pr_err("%s: cmu_busc ioremap failed\n", __func__);
}

void (*cal_data_init)(void) = exynos9820_cal_data_init;

#if defined(CONFIG_SEC_DEBUG)
int asv_ids_information(enum ids_info id)
{
	int res;

	switch (id) {
	case tg:
		res = asv_get_table_ver();
		break;
	case lg:
		res = asv_get_grp(dvfs_cpucl0);
		break;
	case bg:
		res = asv_get_grp(dvfs_cpucl1);
		break;
	case g3dg:
		res = asv_get_grp(dvfs_g3d);
		break;
	case mifg:
		res = asv_get_grp(dvfs_mif);
		break;
	case bids:
		res = asv_get_ids_info(dvfs_cpucl1);
		break;
	case gids:
		res = asv_get_ids_info(dvfs_g3d);
		break;
	default:
		res = 0;
		break;
	};
	return res;
}
#endif

int exynos9820_set_cmuewf(unsigned int index, unsigned int en, void *cmu_cmu, int *ewf_refcnt)
{
	unsigned int reg;
	int ret = 0;
	int tmp;

	if (en) {
		reg = __raw_readl(cmu_cmu + EARLY_WAKEUP_FORCED_ENABLE);
		reg |= 1 << index;
		__raw_writel(reg, cmu_cmu + EARLY_WAKEUP_FORCED_ENABLE);

		reg = __raw_readl(cmu_busc + QCH_CON_TREX_D0_BUSC_QCH_OFFSET);
		reg |= 1 << IGNORE_FORCE_PM_EN;
		__raw_writel(reg, cmu_busc + QCH_CON_TREX_D0_BUSC_QCH_OFFSET);

		ewf_refcnt[index] += 1;
	} else {

		tmp = ewf_refcnt[index] - 1;

		if (tmp == 0) {
			reg = __raw_readl(cmu_busc + QCH_CON_TREX_D0_BUSC_QCH_OFFSET);
			reg &= ~(1 << IGNORE_FORCE_PM_EN);
			__raw_writel(reg, cmu_busc + QCH_CON_TREX_D0_BUSC_QCH_OFFSET);

			reg = __raw_readl(cmu_cmu + EARLY_WAKEUP_FORCED_ENABLE);
			reg &= ~(1 << index);
			__raw_writel(reg, cmu_cmu + EARLY_WAKEUP_FORCED_ENABLE);
		} else if (tmp < 0) {
			pr_err("[EWF]%s ref count mismatch. ewf_index:%u\n",__func__,  index);

			ret = -EINVAL;
			goto exit;
		}

		ewf_refcnt[index] -= 1;
	}

exit:
	return ret;
}
int (*wa_set_cmuewf)(unsigned int index, unsigned int en, void *cmu_cmu, int *ewf_refcnt) = exynos9820_set_cmuewf;

int exynos9820_cal_check_hiu_dvfs_id(u32 id)
{
	if ((IS_ENABLED(CONFIG_EXYNOS_PSTATE_HAFM) || IS_ENABLED(CONFIG_EXYNOS_PSTATE_HAFM_TB)) && id == dvfs_cpucl2)
		return 1;
	else
		return 0;
}
int (*cal_check_hiu_dvfs_id)(u32 id) = exynos9820_cal_check_hiu_dvfs_id;

void (*cal_set_cmu_smpl_warn)(void);
