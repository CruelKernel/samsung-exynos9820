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
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("gate_isp_sysmmu"),
	REGISTER_CLK("gate_isp_ppmu"),
	REGISTER_CLK("gate_isp_bts"),
	REGISTER_CLK("gate_isp_cam"),
	REGISTER_CLK("gate_isp_isp"),
	REGISTER_CLK("gate_isp_vra"),
	REGISTER_CLK("pxmxdx_isp_isp"),
	REGISTER_CLK("pxmxdx_isp_cam"),
	REGISTER_CLK("pxmxdx_isp_vra"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user"),
	REGISTER_CLK("umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user"),
	REGISTER_CLK("isp_sensor0_sclk"),
	REGISTER_CLK("isp_sensor1_sclk"),
	REGISTER_CLK("isp_sensor2_sclk"),
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

/* for debug */
void __iomem *cmu_mif;
void __iomem *cmu_isp;

static void exynos7870_fimc_is_print_clk_reg(void)
{
	info("MIF\n");
	PRINT_CLK((cmu_mif + 0x0140), "BUS_PLL_CON0");
	PRINT_CLK((cmu_mif + 0x0144), "BUS_PLL_CON1");
	PRINT_CLK((cmu_mif + 0x0208), "CLK_CON_MUX_BUS_PLL");
	PRINT_CLK((cmu_mif + 0x0264), "CLK_CON_MUX_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0268), "CLK_CON_MUX_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x026C), "CLK_CON_MUX_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x02C4), "CLK_CON_MUX_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x02C8), "CLK_CON_MUX_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x02CC), "CLK_CON_MUX_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_mif + 0x0464), "CLK_CON_DIV_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0468), "CLK_CON_DIV_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x046C), "CLK_CON_DIV_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x04C4), "CLK_CON_DIV_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x04C8), "CLK_CON_DIV_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x04CC), "CLK_CON_DIV_CLKCMU_ISP_SENSOR2");
	PRINT_CLK((cmu_mif + 0x0864), "CLK_ENABLE_CLKCMU_ISP_VRA");
	PRINT_CLK((cmu_mif + 0x0868), "CLK_ENABLE_CLKCMU_ISP_CAM");
	PRINT_CLK((cmu_mif + 0x086C), "CLK_ENABLE_CLKCMU_ISP_ISP");
	PRINT_CLK((cmu_mif + 0x08C4), "CLK_ENABLE_CLKCMU_ISP_SENSOR0");
	PRINT_CLK((cmu_mif + 0x08C8), "CLK_ENABLE_CLKCMU_ISP_SENSOR1");
	PRINT_CLK((cmu_mif + 0x08CC), "CLK_ENABLE_CLKCMU_ISP_SENSOR2");

	info("ISP\n");
	PRINT_CLK((cmu_isp +  0x0100), "ISP_PLL_CON0");
	PRINT_CLK((cmu_isp +  0x0104), "ISP_PLL_CON1");
	PRINT_CLK((cmu_isp +  0x0200), "CLK_CON_MUX_ISP_PLL");
	PRINT_CLK((cmu_isp +  0x0210), "CLK_CON_MUX_CLKCMU_ISP_VRA_USER");
	PRINT_CLK((cmu_isp +  0x0214), "CLK_CON_MUX_CLKCMU_ISP_CAM_USER");
	PRINT_CLK((cmu_isp +  0x0218), "CLK_CON_MUX_CLKCMU_ISP_ISP_USER");
	PRINT_CLK((cmu_isp +  0x0220), "CLK_CON_MUX_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0224), "CLK_CON_MUX_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0228), "CLK_CON_MUX_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x022C), "CLK_CON_MUX_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x0230), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4_USER");
	PRINT_CLK((cmu_isp +  0x0234), "CLK_CON_MUX_CLKPHY_ISP_S_RXBYTECLKHS0_S4S_USER");
	PRINT_CLK((cmu_isp +  0x0400), "CLK_CON_DIV_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0404), "CLK_CON_DIV_CLK_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0810), "CLK_ENABLE_CLK_ISP_VRA");
	PRINT_CLK((cmu_isp +  0x0814), "CLK_ENABLE_CLK_ISP_APB");
	PRINT_CLK((cmu_isp +  0x0818), "CLK_ENABLE_CLK_ISP_ISPD");
	PRINT_CLK((cmu_isp +  0x081C), "CLK_ENABLE_CLK_ISP_CAM");
	PRINT_CLK((cmu_isp +  0x0820), "CLK_ENABLE_CLK_ISP_CAM_HALF");
	PRINT_CLK((cmu_isp +  0x0824), "CLK_ENABLE_CLK_ISP_ISP");
	PRINT_CLK((cmu_isp +  0x0828), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4");
	PRINT_CLK((cmu_isp +  0x082C), "CLK_ENABLE_CLKPHY_ISP_S_RXBYTECLKHS0_S4S");
}

static int exynos7870_fimc_is_print_clk(struct device *dev)
{
	info("#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "isp_sensor0_sclk");
	fimc_is_get_rate(dev, "isp_sensor1_sclk");
	fimc_is_get_rate(dev, "isp_sensor2_sclk");

	info("#################### ISP clock ####################\n");
	fimc_is_get_rate(dev, "gate_isp_sysmmu");
	fimc_is_get_rate(dev, "gate_isp_ppmu");
	fimc_is_get_rate(dev, "gate_isp_bts");
	fimc_is_get_rate(dev, "gate_isp_cam");
	fimc_is_get_rate(dev, "gate_isp_isp");
	fimc_is_get_rate(dev, "gate_isp_vra");
	fimc_is_get_rate(dev, "pxmxdx_isp_isp");
	fimc_is_get_rate(dev, "pxmxdx_isp_cam");
	fimc_is_get_rate(dev, "pxmxdx_isp_vra");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
	fimc_is_get_rate(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");

	exynos7870_fimc_is_print_clk_reg();

	return 0;
}


int exynos7870_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos7870_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos7870_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	u32 index;

	/* for debug */
	cmu_mif = ioremap(0x10460000, SZ_4K);
	cmu_isp = ioremap(0x144D0000, SZ_4K);

	for (index = 0; index < ARRAY_SIZE(fimc_is_clk_list); ++index) {
		name = fimc_is_clk_list[index].name;
		if (!name) {
			pr_err("[@][ERR] %s: name is NULL\n", __func__);
			return -EINVAL;
		}

		clk = devm_clk_get(dev, name);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("[@][ERR] %s: could not lookup clock : %s\n", __func__, name);
			return -EINVAL;
		}

		fimc_is_clk_list[index].clk = clk;
	}

	return 0;
}

int exynos7870_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos7870_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos7870_fimc_is_clk_on(struct device *dev)
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

	/* ISP */
	fimc_is_enable(dev, "gate_isp_sysmmu");
	fimc_is_enable(dev, "gate_isp_ppmu");
	fimc_is_enable(dev, "gate_isp_bts");
	fimc_is_enable(dev, "gate_isp_cam");
	fimc_is_enable(dev, "gate_isp_isp");
	fimc_is_enable(dev, "gate_isp_vra");
	fimc_is_enable(dev, "pxmxdx_isp_isp");
	fimc_is_enable(dev, "pxmxdx_isp_cam");
	fimc_is_enable(dev, "pxmxdx_isp_vra");

#ifdef DBG_DUMPCMU
	exynos7870_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos7870_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* ISP */
	fimc_is_disable(dev, "gate_isp_sysmmu");
	fimc_is_disable(dev, "gate_isp_ppmu");
	fimc_is_disable(dev, "gate_isp_bts");
	fimc_is_disable(dev, "gate_isp_cam");
	fimc_is_disable(dev, "gate_isp_isp");
	fimc_is_disable(dev, "gate_isp_vra");
	fimc_is_disable(dev, "pxmxdx_isp_isp");
	fimc_is_disable(dev, "pxmxdx_isp_cam");
	fimc_is_disable(dev, "pxmxdx_isp_vra");

	pdata->clock_on = false;

p_err:
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
	exynos7870_fimc_is_get_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
	exynos7870_fimc_is_cfg_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
	exynos7870_fimc_is_clk_on(dev);
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
	exynos7870_fimc_is_clk_off(dev);
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
	exynos7870_fimc_is_print_clk(dev);
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
	exynos7870_fimc_is_clk_gate(clk_gate_id, is_on);
	return 0;
}
