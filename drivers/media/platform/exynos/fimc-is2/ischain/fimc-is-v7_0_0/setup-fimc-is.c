/* linux/arch/arm/mach-exynos/setup-fimc-is.c
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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

#include <soc/samsung/exynos-pd.h>

#include <exynos-fimc-is.h>
#include <fimc-is-config.h>

/*------------------------------------------------------*/
/*		Common control				*/
/*------------------------------------------------------*/

#define REGISTER_CLK(name) {name, NULL}

struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("GATE_ISPHQ_CMU_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_VGEN_LITE_ISPHQ"),
	REGISTER_CLK("GATE_IS_ISPHQ_ISPHQ_C2COM"),

	REGISTER_CLK("UMUX_CLKCMU_ISPLP_BUS"),
	REGISTER_CLK("UMUX_CLKCMU_ISPLP_GDC"),
	REGISTER_CLK("GATE_IS_ISPLP_MC_SCALER"),
	REGISTER_CLK("GATE_IS_ISPLP_ISPLP"),
	REGISTER_CLK("GATE_IS_ISPLP_GDC"),
	REGISTER_CLK("GATE_IS_ISPLP_VGEN_LITE"),
	REGISTER_CLK("GATE_IS_ISPLP_ISPLP_C2"),

	REGISTER_CLK("UMUX_CLKCMU_ISPPRE_BUS"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSIS0"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSIS1"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSIS2"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSIS3"),
	REGISTER_CLK("GATE_IS_ISPPRE_PDP_TOP_DMA"),
	REGISTER_CLK("GATE_IS_ISPPRE_3AA0"),
	REGISTER_CLK("GATE_IS_ISPPRE_3AA1"),
	REGISTER_CLK("GATE_IS_ISPPRE_PDP_TOP_CORE_TOP"),
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	REGISTER_CLK("GATE_IS_ISPPRE_CSISX4_PDP_RDMA"),
#endif
	REGISTER_CLK("GATE_IS_ISPPRE_VGEN_LITE"),
	REGISTER_CLK("GATE_IS_ISPPRE_VGEN_LITE1"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSIS4"),
	REGISTER_CLK("GATE_IS_ISPPRE_CSISX4_PDP_DMA"),
	REGISTER_CLK("GATE_IS_ISPPRE_VGEN_LITE2"),
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	REGISTER_CLK("GATE_IS_ISPPRE_PAFSTAT0"),
	REGISTER_CLK("GATE_IS_ISPPRE_PAFSTAT1"),
#endif

	REGISTER_CLK("UMUX_CLKCMU_VRA2_BUS"),
	REGISTER_CLK("GATE_VRA2"),
	REGISTER_CLK("DOUT_CLK_VRA2_BUSP"),

	REGISTER_CLK("CIS_CLK0"),
	REGISTER_CLK("CIS_CLK1"),
	REGISTER_CLK("CIS_CLK2"),
	REGISTER_CLK("CIS_CLK3"),
	REGISTER_CLK("CIS_CLK3"),

	REGISTER_CLK("MUX_CIS_CLK0"),
	REGISTER_CLK("MUX_CIS_CLK1"),
	REGISTER_CLK("MUX_CIS_CLK2"),
	REGISTER_CLK("MUX_CIS_CLK3"),
	REGISTER_CLK("MUX_CIS_CLK4"),
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

#ifdef CONFIG_SOC_EXYNOS9820
int is_enabled_clk_disable(struct device *dev, const char *name)
{
	int i;
	struct clk *clk = NULL;

	for (i = 0; i < ARRAY_SIZE(fimc_is_clk_list); i++) {
		if (!strcmp(name, fimc_is_clk_list[i].name))
			clk = fimc_is_clk_list[i].clk;
	}

	if (IS_ERR_OR_NULL(clk)) {
		pr_err("%s: failed to find clk: %s\n", __func__, name);
		return -EINVAL;
	}

	if (__clk_get_enable_count(clk)) {
		pr_err("%s: abnormal clock state is detected: %s, %d\n",
				__func__, name,  __clk_get_enable_count(clk));
		clk_disable_unprepare(clk);
	}

	return 0;
}
#endif

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
		clk_put(p);
		pr_err("%s: could not lookup clock : %s\n", __func__, child);
		return -EINVAL;
	}

	ret = clk_set_parent(c, p);
	if (ret) {
		clk_put(c);
		clk_put(p);
		pr_err("%s: clk_set_parent is fail(%s -> %s)(ret: %d)\n", __func__, child, parent, ret);
		return ret;
	}

	clk_put(c);
	clk_put(p);

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

	clk_put(target);

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

	clk_put(target);

	pr_info("[@] %s: %ldMhz\n", conid, rate_target/1000000);

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

	clk_put(target);

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
	clk_put(target);

	return 0;
}

static void exynos9820_fimc_is_print_clk_reg(void)
{

}

static int exynos9820_fimc_is_print_clk(struct device *dev)
{
	struct exynos_pm_domain *exynos_pd;

	pr_info("FIMC-IS CLOCK DUMP\n");

	exynos_pd = exynos_pd_lookup_name("pd-isppre");
	if (exynos_pd_status(exynos_pd) == 1) {
		fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPPRE_BUS");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSIS0");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSIS1");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSIS2");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSIS3");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_PDP_TOP_DMA");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_3AA0");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_3AA1");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_PDP_TOP_CORE_TOP");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSISX4_PDP_RDMA");
#endif
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_VGEN_LITE");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_VGEN_LITE1");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSIS4");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_CSISX4_PDP_DMA");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_VGEN_LITE2");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_PAFSTAT0");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPPRE_PAFSTAT1");
#endif

		fimc_is_get_rate_dt(dev, "CIS_CLK0");
		fimc_is_get_rate_dt(dev, "CIS_CLK1");
		fimc_is_get_rate_dt(dev, "CIS_CLK2");
		fimc_is_get_rate_dt(dev, "CIS_CLK3");
		fimc_is_get_rate_dt(dev, "CIS_CLK4");
	}

	exynos_pd = exynos_pd_lookup_name("pd-isplp");
	if (exynos_pd_status(exynos_pd) == 1) {
		fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPLP_BUS");
		fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_ISPLP_GDC");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_MC_SCALER");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_ISPLP");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_GDC");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_VGEN_LITE");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPLP_ISPLP_C2");
	}

	exynos_pd = exynos_pd_lookup_name("pd-isphq");
	if (exynos_pd_status(exynos_pd) == 1) {
		fimc_is_get_rate_dt(dev, "GATE_ISPHQ_CMU_ISPHQ");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_ISPHQ");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_VGEN_LITE_ISPHQ");
		fimc_is_get_rate_dt(dev, "GATE_IS_ISPHQ_ISPHQ_C2COM");
	}

	exynos_pd = exynos_pd_lookup_name("pd-vra2");
	if (exynos_pd_status(exynos_pd) == 1) {
		fimc_is_get_rate_dt(dev, "UMUX_CLKCMU_VRA2_BUS");
		fimc_is_get_rate_dt(dev, "GATE_VRA2");
		fimc_is_get_rate_dt(dev, "DOUT_CLK_VRA2_BUSP");
	}

	exynos9820_fimc_is_print_clk_reg();

	return 0;
}

int exynos9820_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos9820_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos9820_fimc_is_get_clk(struct device *dev)
{
	struct clk *clk;
	size_t i;

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

int exynos9820_fimc_is_cfg_clk(struct device *dev)
{
	return 0;
}

int exynos9820_fimc_is_clk_on(struct device *dev)
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

	fimc_is_enable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_VGEN_LITE_ISPHQ");
	fimc_is_enable(dev, "GATE_IS_ISPHQ_ISPHQ_C2COM");

	fimc_is_enable(dev, "UMUX_CLKCMU_ISPLP_BUS");
	fimc_is_enable(dev, "UMUX_CLKCMU_ISPLP_GDC");
	fimc_is_enable(dev, "GATE_IS_ISPLP_MC_SCALER");
	fimc_is_enable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_enable(dev, "GATE_IS_ISPLP_GDC");
	fimc_is_enable(dev, "GATE_IS_ISPLP_VGEN_LITE");
	fimc_is_enable(dev, "GATE_IS_ISPLP_ISPLP_C2");

	fimc_is_enable(dev, "UMUX_CLKCMU_ISPPRE_BUS");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_3AA0");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_3AA1");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_PDP_TOP_CORE_TOP");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	fimc_is_enable(dev, "GATE_IS_ISPPRE_CSISX4_PDP_RDMA");
#endif
	fimc_is_enable(dev, "GATE_IS_ISPPRE_VGEN_LITE");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_VGEN_LITE1");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_VGEN_LITE2");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	fimc_is_enable(dev, "GATE_IS_ISPPRE_PAFSTAT0");
	fimc_is_enable(dev, "GATE_IS_ISPPRE_PAFSTAT1");
#endif

	fimc_is_enable(dev, "UMUX_CLKCMU_VRA2_BUS");
	fimc_is_enable(dev, "GATE_VRA2");
	fimc_is_enable(dev, "DOUT_CLK_VRA2_BUSP");

	if (debug_clk > 1)
		exynos9820_fimc_is_print_clk(dev);

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos9820_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_disable(dev, "GATE_ISPHQ_CMU_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_VGEN_LITE_ISPHQ");
	fimc_is_disable(dev, "GATE_IS_ISPHQ_ISPHQ_C2COM");

	fimc_is_disable(dev, "UMUX_CLKCMU_ISPLP_BUS");
	fimc_is_disable(dev, "UMUX_CLKCMU_ISPLP_GDC");
	fimc_is_disable(dev, "GATE_IS_ISPLP_MC_SCALER");
	fimc_is_disable(dev, "GATE_IS_ISPLP_ISPLP");
	fimc_is_disable(dev, "GATE_IS_ISPLP_GDC");
	fimc_is_disable(dev, "GATE_IS_ISPLP_VGEN_LITE");
	fimc_is_disable(dev, "GATE_IS_ISPLP_ISPLP_C2");

	fimc_is_disable(dev, "UMUX_CLKCMU_ISPPRE_BUS");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_3AA0");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_3AA1");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_PDP_TOP_CORE_TOP");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	fimc_is_disable(dev, "GATE_IS_ISPPRE_CSISX4_PDP_RDMA");
#endif
	fimc_is_disable(dev, "GATE_IS_ISPPRE_VGEN_LITE");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_VGEN_LITE1");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_VGEN_LITE2");
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	fimc_is_disable(dev, "GATE_IS_ISPPRE_PAFSTAT0");
	fimc_is_disable(dev, "GATE_IS_ISPPRE_PAFSTAT1");
#endif

	fimc_is_disable(dev, "UMUX_CLKCMU_VRA2_BUS");
	fimc_is_disable(dev, "GATE_VRA2");
	fimc_is_disable(dev, "DOUT_CLK_VRA2_BUSP");

	pdata->clock_on = false;

p_err:
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
	exynos9820_fimc_is_get_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
	exynos9820_fimc_is_cfg_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
	exynos9820_fimc_is_clk_on(dev);
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
	exynos9820_fimc_is_clk_off(dev);
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
	exynos9820_fimc_is_print_clk(dev);
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
	exynos9820_fimc_is_clk_gate(clk_gate_id, is_on);
	return 0;
}
