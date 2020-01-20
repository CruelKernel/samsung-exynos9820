/* linux/arch/arm/mach-exynos/setup-fimc-is.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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
	REGISTER_CLK("UMUX_CLKCMU_CAM_BUS"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS0"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS1"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS2"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS3"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_3AA"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_CORE"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_DMA"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_RDMA"),
	REGISTER_CLK("GATE_IS6P10P0_CAM_QCH_S_CAM_SMMU"),

	REGISTER_CLK("GATE_DFTMUX_TOP_QCH_CLK_CSIS0"),
	REGISTER_CLK("GATE_DFTMUX_TOP_QCH_CLK_CSIS1"),
	REGISTER_CLK("GATE_DFTMUX_TOP_QCH_CLK_CSIS2"),
	REGISTER_CLK("GATE_DFTMUX_TOP_QCH_CLK_CSIS3"),
	REGISTER_CLK("CIS_CLK0"),
	REGISTER_CLK("CIS_CLK1"),
	REGISTER_CLK("CIS_CLK2"),
	REGISTER_CLK("CIS_CLK3"),

	REGISTER_CLK("UMUX_CLKCMU_ISP_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_ISP_GDC"),
	REGISTER_CLK("UMUX_CLKCMU_ISP_VRA"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_ISP"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_MCSC"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_VRA"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_GDC"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP0"),
	REGISTER_CLK("GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP1"),
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
		pr_err("[@][ERR] %s: clk_set_rate is fail(%s)(ret: %d)\n", __func__, name, ret);
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
		pr_err("[@][ERR] %s: clk_prepare_enable is fail(%s)(ret: %d)\n", __func__, name, ret);
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
		pr_err("%s: clk_set_parent is fail(%s -> %s)(ret: %d)\n", __func__, child, parent, ret);
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
		pr_err("%s: clk_set_rate is fail(%s)(ret: %d)\n", __func__, conid, ret);
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
		pr_err("%s: clk_prepare is fail(%s)(ret: %d)\n", __func__, conid, ret);
		return ret;
	}

	ret = clk_enable(target);
	if (ret) {
		pr_err("%s: clk_enable is fail(%s)(ret: %d)\n", __func__, conid, ret);
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

static void exynos9610_fimc_is_print_clk_reg(void)
{

}

static int exynos9610_fimc_is_print_clk(struct device *dev)
{
	pr_info("FIMC-IS CLOCK DUMP\n");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS0");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS1");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS2");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS3");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_3AA");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_CORE");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_DMA");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_RDMA");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_SMMU");

	fimc_is_get_rate_dt(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS0");
	fimc_is_get_rate_dt(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS1");
	fimc_is_get_rate_dt(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS2");
	fimc_is_get_rate_dt(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS3");
	fimc_is_get_rate_dt(dev, "CIS_CLK0");
	fimc_is_get_rate_dt(dev, "CIS_CLK1");
	fimc_is_get_rate_dt(dev, "CIS_CLK2");
	fimc_is_get_rate_dt(dev, "CIS_CLK3");

	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISP_BUS");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISP_GDC");
	fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISP_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_ISP");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_MCSC");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_VRA");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_GDC");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP0");
	fimc_is_get_rate_dt(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP1");

	exynos9610_fimc_is_print_clk_reg();

	return 0;
}

int exynos9610_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos9610_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos9610_fimc_is_get_clk(struct device *dev)
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

int exynos9610_fimc_is_cfg_clk(struct device *dev)
{
	return 0;
}

int exynos9610_fimc_is_clk_on(struct device *dev)
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
	fimc_is_enable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS0");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS1");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS2");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS3");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_3AA");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_CORE");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_DMA");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_RDMA");
	fimc_is_enable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_SMMU");

	fimc_is_enable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS0");
	fimc_is_enable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS1");
	fimc_is_enable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS2");
	fimc_is_enable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS3");
	fimc_is_enable(dev, "CIS_CLK0");
	fimc_is_enable(dev, "CIS_CLK1");
	fimc_is_enable(dev, "CIS_CLK2");
	fimc_is_enable(dev, "CIS_CLK3");

	fimc_is_enable(dev, "UMUX_CLKCMU_ISP_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISP_GDC");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISP_VRA");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_ISP");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_MCSC");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_VRA");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_GDC");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP0");
	fimc_is_enable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP1");

	if (debug_clk > 1)
		exynos9610_fimc_is_print_clk(dev);

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos9610_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_disable(dev, "UMUX_CLKCMU_CAM_BUS");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS0");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS1");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS2");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_CSIS3");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_3AA");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_CORE");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_PDP_DMA");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_RDMA");
	fimc_is_disable(dev, "GATE_IS6P10P0_CAM_QCH_S_CAM_SMMU");

	fimc_is_disable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS0");
	fimc_is_disable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS1");
	fimc_is_disable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS2");
	fimc_is_disable(dev, "GATE_DFTMUX_TOP_QCH_CLK_CSIS3");
	fimc_is_disable(dev, "CIS_CLK0");
	fimc_is_disable(dev, "CIS_CLK1");
	fimc_is_disable(dev, "CIS_CLK2");
	fimc_is_disable(dev, "CIS_CLK3");

	fimc_is_disable(dev, "UMUX_CLKCMU_ISP_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISP_GDC");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISP_VRA");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_ISP");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_MCSC");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_VRA");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_GDC");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP0");
	fimc_is_disable(dev, "GATE_IS6P10P0_ISP_QCH_S_ISP_SMMU_ISP1");

	pdata->clock_on = false;

p_err:
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
	exynos9610_fimc_is_get_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
	exynos9610_fimc_is_cfg_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
	exynos9610_fimc_is_clk_on(dev);
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
	exynos9610_fimc_is_clk_off(dev);
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
	exynos9610_fimc_is_print_clk(dev);
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
	exynos9610_fimc_is_clk_gate(clk_gate_id, is_on);
	return 0;
}
