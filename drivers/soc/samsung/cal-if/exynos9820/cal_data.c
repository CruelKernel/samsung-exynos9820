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
void __iomem *cmu_cpucl0;
void __iomem *cmu_cpucl1;
void __iomem *cmu_cpucl2;
void __iomem *cmu_g3d;

#define PLL_CON0_PLL_MMC	(0x1a0)
#define PLL_CON1_PLL_MMC	(0x1a4)
#define PLL_CON2_PLL_MMC	(0x1a8)
#define PLL_CON3_PLL_MMC	(0x1ac)

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

#define EXYNOS9820_CMU_MMC_BASE		(0x13C00000)
#define EXYNOS9820_CMU_BUSC_BASE	(0x1A200000)

#define EXYNOS9820_CMU_CPUCL0_BASE	(0x1D020000)
#define EXYNOS9820_CMU_CPUCL1_BASE	(0x1D030000)
#define EXYNOS9820_CMU_CPUCL2_BASE	(0x1D120000)
#define EXYNOS9820_CMU_G3D_BASE		(0x18400000)

#define EXYNOS9820_CPUCL0_CLKDIV2PH		(0x0840)
#define EXYNOS9820_CPUCL1_CLKDIV2PH		(0x0830)
#define EXYNOS9820_CPUCL2_CTRL_EXT_EVENT	(0x0838)
#define EXYNOS9820_G3D_CLKDIV2PH		(0x0824)

#define MASK_SMPL_INT				(7)
#define ENABLE					(0)

#define SET_1BIT(reg, bit, value)		((reg & ~(1 << bit)) | ((value & 0x1) << bit))

#define QCH_CON_TREX_D0_BUSC_QCH_OFFSET	(0x31a8)
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

	cmu_cpucl0 = ioremap(EXYNOS9820_CMU_CPUCL0_BASE, SZ_4K);
	if (!cmu_cpucl0)
		pr_err("%s: cmu_cpucl0 ioremap failed\n", __func__);

	cmu_cpucl1 = ioremap(EXYNOS9820_CMU_CPUCL1_BASE, SZ_4K);
	if (!cmu_cpucl1)
		pr_err("%s: cmu_cpucl1 ioremap failed\n", __func__);

	cmu_cpucl2 = ioremap(EXYNOS9820_CMU_CPUCL2_BASE, SZ_4K);
	if (!cmu_cpucl2)
		pr_err("%s: cmu_cpucl2 ioremap failed\n", __func__);

	cmu_g3d = ioremap(EXYNOS9820_CMU_G3D_BASE, SZ_4K);
	if (!cmu_g3d)
		pr_err("%s: cmu_g3d ioremap failed\n", __func__);
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
	case mg:
		res = asv_get_grp(dvfs_cpucl1);
		break;
	case bg:
		res = asv_get_grp(dvfs_cpucl2);
		break;
	case g3dg:
		res = asv_get_grp(dvfs_g3d);
		break;
	case mifg:
		res = asv_get_grp(dvfs_mif);
		break;
	case lids:
		res = asv_get_ids_info(dvfs_cpucl0);
		break;
	case mids:
		res = asv_get_ids_info(dvfs_cpucl1);
		break;
	case bids:
		res = asv_get_ids_info(dvfs_cpucl2);
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

void exynos9820_set_cmu_smpl_warn(void)
{
	unsigned int reg;

	reg = __raw_readl(cmu_cpucl1 + EXYNOS9820_CPUCL1_CLKDIV2PH);
	reg = SET_1BIT(reg, MASK_SMPL_INT, 0);
	__raw_writel(reg, cmu_cpucl1 + EXYNOS9820_CPUCL1_CLKDIV2PH);

	reg = __raw_readl(cmu_cpucl2 + EXYNOS9820_CPUCL2_CTRL_EXT_EVENT);
	reg = SET_1BIT(reg, MASK_SMPL_INT, 0);
	__raw_writel(reg, cmu_cpucl2 + EXYNOS9820_CPUCL2_CTRL_EXT_EVENT);

	reg = __raw_readl(cmu_g3d + EXYNOS9820_G3D_CLKDIV2PH);
	reg = SET_1BIT(reg, MASK_SMPL_INT, 0);
	__raw_writel(reg, cmu_g3d + EXYNOS9820_G3D_CLKDIV2PH);

	reg = __raw_readl(cmu_cpucl0 + EXYNOS9820_CPUCL0_CLKDIV2PH);
	reg = SET_1BIT(reg, ENABLE, 1);
	__raw_writel(reg, cmu_cpucl0 + EXYNOS9820_CPUCL0_CLKDIV2PH);
	pr_info("CPUCL0 SMPL_WARN enabled.\n");

	reg = __raw_readl(cmu_cpucl1 + EXYNOS9820_CPUCL1_CLKDIV2PH);
	reg = SET_1BIT(reg, ENABLE, 1);
	__raw_writel(reg, cmu_cpucl1 + EXYNOS9820_CPUCL1_CLKDIV2PH);
	pr_info("CPUCL1 SMPL_WARN enabled.\n");

	reg = __raw_readl(cmu_cpucl2 + EXYNOS9820_CPUCL2_CTRL_EXT_EVENT);
	reg = SET_1BIT(reg, ENABLE, 1);
	__raw_writel(reg, cmu_cpucl2 + EXYNOS9820_CPUCL2_CTRL_EXT_EVENT);
	pr_info("CPUCL2 SMPL_WARN enabled.\n");

	reg = __raw_readl(cmu_g3d + EXYNOS9820_G3D_CLKDIV2PH);
	reg = SET_1BIT(reg, ENABLE, 1);
	__raw_writel(reg, cmu_g3d + EXYNOS9820_G3D_CLKDIV2PH);
	pr_info("G3D SMPL_WARN enabled.\n");
}
void (*cal_set_cmu_smpl_warn)(void) = exynos9820_set_cmu_smpl_warn;

#if defined(CONFIG_SEC_PM_DEBUG) && defined(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>

#define ASV_SUMMARY_SZ	(dvfs_mfc - dvfs_mif + 1)

static int asv_summary_show(struct seq_file *s, void *d)
{
	unsigned int i;
	const char *label[ASV_SUMMARY_SZ] = { "MIF", "INT", "CL0", "CL1", "CL2",
		"NPU", "DISP", "SCORE", "AUD", "CP", "G3D", "INTCAM", "CAM",
		"IVA", "MFC" };

	seq_printf(s, "Table ver: %d\n", asv_get_table_ver());

	for (i = 0; i < ASV_SUMMARY_SZ ; i++)
		seq_printf(s, "%s: %d\n", label[i], asv_get_grp(dvfs_mif + i));

	seq_printf(s, "IDS (b,m,l,g): %d, %d, %d, %d\n",
			asv_get_ids_info(dvfs_cpucl2),
			asv_get_ids_info(dvfs_cpucl1),
			asv_get_ids_info(dvfs_cpucl0),
			asv_get_ids_info(dvfs_g3d));
	return 0;
}

static int asv_summary_open(struct inode *inode, struct file *file)
{
	return single_open(file, asv_summary_show, inode->i_private);
}

const static struct file_operations asv_summary_fops = {
	.open		= asv_summary_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init cal_data_late_init(void)
{
	debugfs_create_file("asv_summary", 0444, NULL, NULL, &asv_summary_fops);

	return 0;
}

late_initcall(cal_data_late_init);
#endif /* CONFIG_SEC_PM_DEBUG && CONFIG_DEBUG_FS */
