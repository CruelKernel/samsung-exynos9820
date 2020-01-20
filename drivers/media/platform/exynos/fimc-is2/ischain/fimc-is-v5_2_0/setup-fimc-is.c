/* linux/arch/arm/mach-exynos/setup-fimc-is.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * FIMC-IS gpio and clock configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

#include <exynos-fimc-is.h>
#include <fimc-is-config.h>

/*------------------------------------------------------*/
/*		Common control				*/
/*------------------------------------------------------*/

#define REGISTER_CLK(name) {name, NULL}

struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("GATE_ISP_EWGEN_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_3AA"),
	REGISTER_CLK("GATE_IS_ISPHQ_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_QE_3AA"),
	REGISTER_CLK("GATE_IS_ISPHQ_QE_ISPHQ"),
	REGISTER_CLK("GATE_ISPHQ_CMU_ISPHQ"),
	REGISTER_CLK("GATE_PMU_ISPHQ"),
	REGISTER_CLK("GATE_SYSREG_ISPHQ"),
	REGISTER_CLK("UMUX_CLKCMU_ISPHQ_BUS"),

	REGISTER_CLK("GATE_ISP_EWGEN_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_3AAW"),
	REGISTER_CLK("GATE_IS_ISPLP_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_QE_3AAW"),
	REGISTER_CLK("GATE_IS_ISPLP_QE_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_SMMU_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_BCM_ISPLP"),
	REGISTER_CLK("GATE_BTM_ISPLP"),
	REGISTER_CLK("GATE_ISPLP_CMU_ISPLP"),
	REGISTER_CLK("GATE_PMU_ISPLP"),
	REGISTER_CLK("GATE_SYSREG_ISPLP"),
	REGISTER_CLK("UMUX_CLKCMU_ISPLP_BUS"),

	REGISTER_CLK("GATE_ISP_EWGEN_CAM"),
	REGISTER_CLK("GATE_IS_CAM_CSIS0"),
	REGISTER_CLK("GATE_IS_CAM_CSIS1"),
	REGISTER_CLK("GATE_IS_CAM_CSIS2"),
	REGISTER_CLK("GATE_IS_CAM_CSIS3"),
	REGISTER_CLK("GATE_IS_CAM_MC_SCALER"),
	REGISTER_CLK("GATE_IS_CAM_CSISX4_DMA"),
	REGISTER_CLK("GATE_IS_CAM_SYSMMU_CAM0"),
	REGISTER_CLK("GATE_IS_CAM_SYSMMU_CAM1"),
	REGISTER_CLK("GATE_IS_CAM_BCM_CAM0"),
	REGISTER_CLK("GATE_IS_CAM_BCM_CAM1"),
	REGISTER_CLK("GATE_IS_CAM_TPU0"),
	REGISTER_CLK("GATE_IS_CAM_VRA"),
	REGISTER_CLK("GATE_IS_CAM_QE_TPU0"),
	REGISTER_CLK("GATE_IS_CAM_QE_VRA"),
	REGISTER_CLK("GATE_IS_CAM_BNS"),
	REGISTER_CLK("GATE_IS_CAM_QE_CSISX4"),
	REGISTER_CLK("GATE_IS_CAM_QE_TPU1"),
	REGISTER_CLK("GATE_IS_CAM_TPU1"),
	REGISTER_CLK("GATE_BTM_CAMD0"),
	REGISTER_CLK("GATE_BTM_CAMD1"),
	REGISTER_CLK("GATE_CAM_CMU_CAM"),
	REGISTER_CLK("GATE_PMU_CAM"),
	REGISTER_CLK("GATE_SYSREG_CAM"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_TPU0"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_VRA"),
	REGISTER_CLK("UMUX_CLKCMU_CAM_TPU1"),

	REGISTER_CLK("MUX_CIS_CLK0"),
	REGISTER_CLK("MUX_CIS_CLK1"),
	REGISTER_CLK("MUX_CIS_CLK2"),
	REGISTER_CLK("MUX_CIS_CLK3"),

	REGISTER_CLK("CIS_CLK0"),
	REGISTER_CLK("CIS_CLK1"),
	REGISTER_CLK("CIS_CLK2"),
	REGISTER_CLK("CIS_CLK3"),
};


int fimc_is_set_rate(struct device *dev,
	const char *name, ulong frequency)
{
	int ret = 0;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	ret = clk_set_rate(clk, frequency);
	if (ret) {
		pr_err("[@][ERR] %s: clk_set_rate is fail(%s)\n", __func__, name);
		return ret;
	}

	/* fimc_is_get_rate_dt(dev, name); */

	return ret;
}

/* utility function to get rate with DT */
ulong fimc_is_get_rate(struct device *dev,
	const char *name)
{
	ulong frequency;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	frequency = clk_get_rate(clk);

	pr_info("[@] %s : %ldMhz (enable_count : %d)\n", name, frequency/1000000, __clk_get_enable_count(clk));

	return frequency;
}

/* utility function to eable with DT */
int  fimc_is_enable(struct device *dev,
	const char *name)
{
	int ret = 0;
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	if (debug_clk > 1)
		pr_info("[@][ENABLE] %s : (enable_count : %d)\n", name, __clk_get_enable_count(clk));

	ret = clk_prepare_enable(clk);
	if (ret) {
		pr_err("[@][ERR] %s: clk_prepare_enable is fail(%s)\n", __func__, name);
		return ret;
	}

	return ret;
}

/* utility function to disable with DT */
int fimc_is_disable(struct device *dev,
	const char *name)
{
	size_t index;
	struct clk *clk = NULL;

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); index++) {
		if (!strcmp(name, fimc_is_clk_list[index].name))
			clk = fimc_is_clk_list[index].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("[@][ERR] %s: clk_target_list is NULL : %s\n", __func__, name);
		return -EINVAL;
	}

	clk_disable_unprepare(clk);

	if (debug_clk > 1)
		pr_info("[@][DISABLE] %s : (enable_count : %d)\n", name, __clk_get_enable_count(clk));

	return 0;
}

/* utility function to set parent with DT */
int fimc_is_set_parent_dt(struct device *dev,
	const char *child, const char *parent)
{
	int ret = 0;
	struct clk *p;
	struct clk *c;

	p = clk_get(dev, parent);
	if (IS_ERR_OR_NULL(p)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, parent);
		return -EINVAL;
	}

	c = clk_get(dev, child);
	if (IS_ERR_OR_NULL(c)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, child);
		return -EINVAL;
	}

	ret = clk_set_parent(c, p);
	if (ret) {
		pr_err("%s: clk_set_parent is fail(%s -> %s)\n", __func__, child, parent);
		return ret;
	}

	return 0;
}

/* utility function to set rate with DT */
int fimc_is_set_rate_dt(struct device *dev,
	const char *conid, unsigned int rate)
{
	int ret = 0;
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	ret = clk_set_rate(target, rate);
	if (ret) {
		pr_err("%s: clk_set_rate is fail(%s)\n", __func__, conid);
		return ret;
	}

	/* fimc_is_get_rate_dt(dev, conid); */

	return 0;
}

/* utility function to get rate with DT */
ulong fimc_is_get_rate_dt(struct device *dev,
	const char *conid)
{
	struct clk *target;
	ulong rate_target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	rate_target = clk_get_rate(target);
	pr_info("[@] %s : %ldMhz\n", conid, rate_target/1000000);

	return rate_target;
}

/* utility function to eable with DT */
int fimc_is_enable_dt(struct device *dev,
	const char *conid)
{
	int ret;
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	ret = clk_prepare(target);
	if (ret) {
		pr_err("%s: clk_prepare is fail(%s)\n", __func__, conid);
		return ret;
	}

	ret = clk_enable(target);
	if (ret) {
		pr_err("%s: clk_enable is fail(%s)\n", __func__, conid);
		return ret;
	}

	return 0;
}

/* utility function to disable with DT */
int fimc_is_disable_dt(struct device *dev,
	const char *conid)
{
	struct clk *target;

	target = clk_get(dev, conid);
	if (IS_ERR_OR_NULL(target)) {
		pr_err("%s: could not lookup clock : %s\n", __func__, conid);
		return -EINVAL;
	}

	clk_disable(target);
	clk_unprepare(target);

	return 0;
}

static void exynos8895_fimc_is_print_clk_reg(void)
{

}

static int exynos8895_fimc_is_print_clk(struct device *dev)
{
	pr_info("FIMC-IS CLOCK DUMP\n");
	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_PMU_ISPHQ");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_BTM_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_PMU_ISPLP");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_ISPLP");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_get_rate_dt(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_TPU0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_BNS");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_get_rate_dt(dev, "GATE_IS_CAM_TPU1");
	fimc_is_get_rate_dt(dev, "GATE_BTM_CAMD0");
	fimc_is_get_rate_dt(dev, "GATE_BTM_CAMD1");
	fimc_is_get_rate_dt(dev, "GATE_CAM_CMU_CAM");
	fimc_is_get_rate_dt(dev, "GATE_PMU_CAM");
	fimc_is_get_rate_dt(dev, "GATE_SYSREG_CAM");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK0");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK1");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK2");
	fimc_is_get_rate_dt(dev, "MUX_CIS_CLK3");

	fimc_is_get_rate_dt(dev, "CIS_CLK0");
	fimc_is_get_rate_dt(dev, "CIS_CLK1");
	fimc_is_get_rate_dt(dev, "CIS_CLK2");
	fimc_is_get_rate_dt(dev, "CIS_CLK3");

	exynos8895_fimc_is_print_clk_reg();

	return 0;
}

int exynos8895_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos8895_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos8895_fimc_is_get_clk(struct device *dev)
{
	struct clk *clk;
	int i;

	for (i = 0; i < ARRAY_SIZE(fimc_is_clk_list); i++) {
		clk = devm_clk_get(dev, fimc_is_clk_list[i].name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n",
				__func__, fimc_is_clk_list[i].name);
			return -EINVAL;
		}

		fimc_is_clk_list[i].clk = clk;
	}

	return 0;
}

int exynos8895_fimc_is_cfg_clk(struct device *dev)
{
	return 0;
}

int exynos8895_fimc_is_clk_on(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (pdata->clock_on) {
		ret = pdata->clk_off(dev);
		if (ret) {
			pr_err("clk_off is fail(%d)\n", ret);
			goto p_err;
		}
	}

	fimc_is_enable(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_enable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_enable(dev, "GATE_PMU_ISPHQ");
	fimc_is_enable(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_enable(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_enable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_enable(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_enable(dev, "GATE_BTM_ISPLP");
	fimc_is_enable(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_enable(dev, "GATE_PMU_ISPLP");
	fimc_is_enable(dev, "GATE_SYSREG_ISPLP");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_enable(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_enable(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_enable(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_enable(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_enable(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_enable(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_enable(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_enable(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_enable(dev, "GATE_IS_CAM_TPU0");
	fimc_is_enable(dev, "GATE_IS_CAM_VRA");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_enable(dev, "GATE_IS_CAM_BNS");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_enable(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_enable(dev, "GATE_IS_CAM_TPU1");
	fimc_is_enable(dev, "GATE_BTM_CAMD0");
	fimc_is_enable(dev, "GATE_BTM_CAMD1");
	fimc_is_enable(dev, "GATE_CAM_CMU_CAM");
	fimc_is_enable(dev, "GATE_PMU_CAM");
	fimc_is_enable(dev, "GATE_SYSREG_CAM");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_enable(dev, "MUX_CIS_CLK0");
	fimc_is_enable(dev, "MUX_CIS_CLK1");
	fimc_is_enable(dev, "MUX_CIS_CLK2");
	fimc_is_enable(dev, "MUX_CIS_CLK3");

	fimc_is_enable(dev, "CIS_CLK0");
	fimc_is_enable(dev, "CIS_CLK1");
	fimc_is_enable(dev, "CIS_CLK2");
	fimc_is_enable(dev, "CIS_CLK3");

	if (debug_clk > 1)
		exynos8895_fimc_is_print_clk(dev);

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos8895_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_disable(dev, "GATE_ISP_EWGEN_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_3AA");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_QE_3AA");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_QE_ISPHQ");
	fimc_is_disable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_disable(dev, "GATE_PMU_ISPHQ");
	fimc_is_disable(dev, "GATE_SYSREG_ISPHQ");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISPHQ_BUS");

	fimc_is_disable(dev, "GATE_ISP_EWGEN_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_3AAW");
	fimc_is_disable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_QE_3AAW");
	fimc_is_disable(dev, "GATE_IS_ISPLP_QE_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_SMMU_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_BCM_ISPLP");
	fimc_is_disable(dev, "GATE_BTM_ISPLP");
	fimc_is_disable(dev, "GATE_ISPLP_CMU_ISPLP");
	fimc_is_disable(dev, "GATE_PMU_ISPLP");
	fimc_is_disable(dev, "GATE_SYSREG_ISPLP");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISPLP_BUS");

	fimc_is_disable(dev, "GATE_ISP_EWGEN_CAM");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS0");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS1");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS2");
	fimc_is_disable(dev, "GATE_IS_CAM_CSIS3");
	fimc_is_disable(dev, "GATE_IS_CAM_MC_SCALER");
	fimc_is_disable(dev, "GATE_IS_CAM_CSISX4_DMA");
	fimc_is_disable(dev, "GATE_IS_CAM_SYSMMU_CAM0");
	fimc_is_disable(dev, "GATE_IS_CAM_SYSMMU_CAM1");
	fimc_is_disable(dev, "GATE_IS_CAM_BCM_CAM0");
	fimc_is_disable(dev, "GATE_IS_CAM_BCM_CAM1");
	fimc_is_disable(dev, "GATE_IS_CAM_TPU0");
	fimc_is_disable(dev, "GATE_IS_CAM_VRA");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_TPU0");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_VRA");
	fimc_is_disable(dev, "GATE_IS_CAM_BNS");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_CSISX4");
	fimc_is_disable(dev, "GATE_IS_CAM_QE_TPU1");
	fimc_is_disable(dev, "GATE_IS_CAM_TPU1");
	fimc_is_disable(dev, "GATE_BTM_CAMD0");
	fimc_is_disable(dev, "GATE_BTM_CAMD1");
	fimc_is_disable(dev, "GATE_CAM_CMU_CAM");
	fimc_is_disable(dev, "GATE_PMU_CAM");
	fimc_is_disable(dev, "GATE_SYSREG_CAM");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_TPU0");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_VRA");
	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_TPU1");

	fimc_is_disable(dev, "MUX_CIS_CLK0");
	fimc_is_disable(dev, "MUX_CIS_CLK1");
	fimc_is_disable(dev, "MUX_CIS_CLK2");
	fimc_is_disable(dev, "MUX_CIS_CLK3");

	fimc_is_disable(dev, "CIS_CLK0");
	fimc_is_disable(dev, "CIS_CLK1");
	fimc_is_disable(dev, "CIS_CLK2");
	fimc_is_disable(dev, "CIS_CLK3");

	pdata->clock_on = false;

p_err:
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
	exynos8895_fimc_is_get_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
	exynos8895_fimc_is_cfg_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
	exynos8895_fimc_is_clk_on(dev);
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
	exynos8895_fimc_is_clk_off(dev);
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
	exynos8895_fimc_is_print_clk(dev);
	return 0;
}

int exynos_fimc_is_set_user_clk_gate(u32 group_id, bool is_on,
	u32 user_scenario_id,
	unsigned long msk_state,
	struct exynos_fimc_is_clk_gate_info *gate_info)
{
	return 0;
}

int exynos_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	exynos8895_fimc_is_clk_gate(clk_gate_id, is_on);
	return 0;
}
