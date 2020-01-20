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

#define PRINT_CLK(c, n) info("%s : 0x%08X\n", n, readl(c))
#define REGISTER_CLK(name) {name, NULL}

struct fimc_is_clk fimc_is_clk_list[] = {
	REGISTER_CLK("oscclk"),
	REGISTER_CLK("gate_fimc_isp0"),
	REGISTER_CLK("gate_fimc_tpu"),
	REGISTER_CLK("isp0"),
	REGISTER_CLK("isp0_tpu"),
	REGISTER_CLK("isp0_trex"),
	REGISTER_CLK("pxmxdx_isp0_pxl"),
	REGISTER_CLK("gate_fimc_isp1"),
	REGISTER_CLK("isp1"),
	REGISTER_CLK("isp_sensor0"),
	REGISTER_CLK("isp_sensor1"),
	REGISTER_CLK("isp_sensor2"),
	REGISTER_CLK("isp_sensor3"),
	REGISTER_CLK("gate_csis0"),
	REGISTER_CLK("gate_csis1"),
	REGISTER_CLK("gate_fimc_bns"),
	REGISTER_CLK("gate_fimc_3aa0"),
	REGISTER_CLK("gate_fimc_3aa1"),
	REGISTER_CLK("gate_hpm"),
	REGISTER_CLK("pxmxdx_csis0"),
	REGISTER_CLK("pxmxdx_csis1"),
	REGISTER_CLK("pxmxdx_csis2"),
	REGISTER_CLK("pxmxdx_csis3"),
	REGISTER_CLK("pxmxdx_3aa0"),
	REGISTER_CLK("pxmxdx_3aa1"),
	REGISTER_CLK("pxmxdx_trex"),
	REGISTER_CLK("hs0_csis0_rx_byte"),
	REGISTER_CLK("hs1_csis0_rx_byte"),
	REGISTER_CLK("hs2_csis0_rx_byte"),
	REGISTER_CLK("hs3_csis0_rx_byte"),
	REGISTER_CLK("hs0_csis1_rx_byte"),
	REGISTER_CLK("hs1_csis1_rx_byte"),
	REGISTER_CLK("gate_isp_cpu"),
	REGISTER_CLK("gate_csis2"),
	REGISTER_CLK("gate_csis3"),
	REGISTER_CLK("gate_fimc_vra"),
	REGISTER_CLK("gate_mc_scaler"),
	REGISTER_CLK("gate_i2c0_isp"),
	REGISTER_CLK("gate_i2c1_isp"),
	REGISTER_CLK("gate_i2c2_isp"),
	REGISTER_CLK("gate_i2c3_isp"),
	REGISTER_CLK("gate_wdt_isp"),
	REGISTER_CLK("gate_mcuctl_isp"),
	REGISTER_CLK("gate_uart_isp"),
	REGISTER_CLK("gate_pdma_isp"),
	REGISTER_CLK("gate_pwm_isp"),
	REGISTER_CLK("gate_spi0_isp"),
	REGISTER_CLK("gate_spi1_isp"),
	REGISTER_CLK("isp_spi0"),
	REGISTER_CLK("isp_spi1"),
	REGISTER_CLK("isp_uart"),
	REGISTER_CLK("gate_sclk_pwm_isp"),
	REGISTER_CLK("gate_sclk_uart_isp"),
	REGISTER_CLK("cam1_arm"),
	REGISTER_CLK("cam1_vra"),
	REGISTER_CLK("cam1_trex"),
	REGISTER_CLK("cam1_bus"),
	REGISTER_CLK("cam1_peri"),
	REGISTER_CLK("cam1_csis2"),
	REGISTER_CLK("cam1_csis3"),
	REGISTER_CLK("cam1_scl"),
	REGISTER_CLK("cam1_phy0_csis2"),
	REGISTER_CLK("cam1_phy1_csis2"),
	REGISTER_CLK("cam1_phy2_csis2"),
	REGISTER_CLK("cam1_phy3_csis2"),
	REGISTER_CLK("cam1_phy0_csis3"),
	REGISTER_CLK("mipi_dphy_m4s4"),
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
void __iomem *cmu_top;
void __iomem *cmu_cam0;
void __iomem *cmu_cam1;
void __iomem *cmu_isp0;
void __iomem *cmu_isp1;

static void exynos8890_fimc_is_print_clk_reg(void)
{
	info("TOP0\n");
	PRINT_CLK((cmu_top + 0x0100), "BUS0_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0120), "BUS1_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0140), "BUS2_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0160), "BUS3_PLL_CON0");
	PRINT_CLK((cmu_top + 0x01A0), "ISP_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0180), "MFC_PLL_CON0");
	PRINT_CLK((cmu_top + 0x0200), "CLK_CON_MUX_BUS0_PLL");
	PRINT_CLK((cmu_top + 0x0204), "CLK_CON_MUX_BUS1_PLL");
	PRINT_CLK((cmu_top + 0x0208), "CLK_CON_MUX_BUS2_PLL");
	PRINT_CLK((cmu_top + 0x020C), "CLK_CON_MUX_BUS3_PLL");
	PRINT_CLK((cmu_top + 0x0210), "CLK_CON_MUX_MFC_PLL");
	PRINT_CLK((cmu_top + 0x0214), "CLK_CON_MUX_ISP_PLL");
	PRINT_CLK((cmu_top + 0x0220), "CLK_CON_MUX_SCLK_BUS0_PLL");
	PRINT_CLK((cmu_top + 0x0224), "CLK_CON_MUX_SCLK_BUS1_PLL");
	PRINT_CLK((cmu_top + 0x0228), "CLK_CON_MUX_SCLK_BUS2_PLL");
	PRINT_CLK((cmu_top + 0x022C), "CLK_CON_MUX_SCLK_BUS3_PLL");
	PRINT_CLK((cmu_top + 0x0230), "CLK_CON_MUX_SCLK_MFC_PLL");
	PRINT_CLK((cmu_top + 0x0234), "CLK_CON_MUX_SCLK_ISP_PLL");
	PRINT_CLK((cmu_top + 0x02B8), "CLK_CON_MUX_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_top + 0x02BC), "CLK_CON_MUX_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x02C0), "CLK_CON_MUX_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x02C4), "CLK_CON_MUX_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x02C8), "CLK_CON_MUX_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x02CC), "CLK_CON_MUX_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x02D0), "CLK_CON_MUX_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x02D4), "CLK_CON_MUX_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x02D8), "CLK_CON_MUX_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x02DC), "CLK_CON_MUX_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x02E0), "CLK_CON_MUX_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x02E4), "CLK_CON_MUX_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x02E8), "CLK_CON_MUX_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x02EC), "CLK_CON_MUX_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x02F0), "CLK_CON_MUX_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x02A8), "CLK_CON_MUX_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x02AC), "CLK_CON_MUX_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x02B0), "CLK_CON_MUX_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x02B4), "CLK_CON_MUX_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x0368), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x036C), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x0370), "CLK_CON_MUX_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B00), "CLK_CON_MUX_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B10), "CLK_CON_MUX_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B20), "CLK_CON_MUX_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B30), "CLK_CON_MUX_SCLK_ISP_SENSOR3");
	PRINT_CLK((cmu_top + 0x0418), "CLK_CON_DIV_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_top + 0x041C), "CLK_CON_DIV_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x0420), "CLK_CON_DIV_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x0424), "CLK_CON_DIV_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x0428), "CLK_CON_DIV_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x042C), "CLK_CON_DIV_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0430), "CLK_CON_DIV_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x0434), "CLK_CON_DIV_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x0438), "CLK_CON_DIV_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x043C), "CLK_CON_DIV_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x0440), "CLK_CON_DIV_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x0444), "CLK_CON_DIV_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x0448), "CLK_CON_DIV_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x044C), "CLK_CON_DIV_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0450), "CLK_CON_DIV_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x0408), "CLK_CON_DIV_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x040C), "CLK_CON_DIV_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x0410), "CLK_CON_DIV_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x0414), "CLK_CON_DIV_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x04C8), "CLK_CON_DIV_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x04CC), "CLK_CON_DIV_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x04D0), "CLK_CON_DIV_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B04), "CLK_CON_DIV_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B14), "CLK_CON_DIV_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B24), "CLK_CON_DIV_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B34), "CLK_CON_DIV_SCLK_ISP_SENSOR3");
	PRINT_CLK((cmu_top + 0x0878), "CLK_ENABLE_ACLK_CAM0_CSIS1_414");
	PRINT_CLK((cmu_top + 0x087C), "CLK_ENABLE_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_top + 0x0880), "CLK_ENABLE_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_top + 0x0884), "CLK_ENABLE_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_top + 0x0888), "CLK_ENABLE_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_top + 0x088C), "CLK_ENABLE_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_top + 0x0890), "CLK_ENABLE_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_top + 0x0894), "CLK_ENABLE_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_top + 0x0898), "CLK_ENABLE_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_top + 0x089C), "CLK_ENABLE_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_top + 0x08A0), "CLK_ENABLE_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_top + 0x08A4), "CLK_ENABLE_ACLK_CAM1_PERI_84");
	PRINT_CLK((cmu_top + 0x08A8), "CLK_ENABLE_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_top + 0x08AC), "CLK_ENABLE_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_top + 0x08B0), "CLK_ENABLE_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_top + 0x0868), "CLK_ENABLE_ACLK_ISP0_ISP0_528");
	PRINT_CLK((cmu_top + 0x086C), "CLK_ENABLE_ACLK_ISP0_TPU_400");
	PRINT_CLK((cmu_top + 0x0870), "CLK_ENABLE_ACLK_ISP0_TREX_528");
	PRINT_CLK((cmu_top + 0x0874), "CLK_ENABLE_ACLK_ISP1_ISP1_468");
	PRINT_CLK((cmu_top + 0x0974), "CLK_ENABLE_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_top + 0x0978), "CLK_ENABLE_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_top + 0x097C), "CLK_ENABLE_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_top + 0x0B0C), "CLK_ENABLE_SCLK_ISP_SENSOR0");
	PRINT_CLK((cmu_top + 0x0B1C), "CLK_ENABLE_SCLK_ISP_SENSOR1");
	PRINT_CLK((cmu_top + 0x0B2C), "CLK_ENABLE_SCLK_ISP_SENSOR2");
	PRINT_CLK((cmu_top + 0x0B3C), "CLK_ENABLE_SCLK_ISP_SENSOR3");

	info("CAM0\n");
	PRINT_CLK((cmu_cam0 +  0x0200), "CLK_CON_MUX_ACLK_CAM0_CSIS0_414_USER");
	PRINT_CLK((cmu_cam0 +  0x0204), "CLK_CON_MUX_ACLK_CAM0_CSIS1_168_USER");
	PRINT_CLK((cmu_cam0 +  0x0208), "CLK_CON_MUX_ACLK_CAM0_CSIS2_234_USER");
	PRINT_CLK((cmu_cam0 +  0x020C), "CLK_CON_MUX_ACLK_CAM0_CSIS3_132_USER");
	PRINT_CLK((cmu_cam0 +  0x0214), "CLK_CON_MUX_ACLK_CAM0_3AA0_414_USER");
	PRINT_CLK((cmu_cam0 +  0x0218), "CLK_CON_MUX_ACLK_CAM0_3AA1_414_USER");
	PRINT_CLK((cmu_cam0 +  0x021C), "CLK_CON_MUX_ACLK_CAM0_TREX_528_USER");
	PRINT_CLK((cmu_cam0 +  0x0220), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0224), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0228), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS2_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x022C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS3_CSIS0_USER");
	PRINT_CLK((cmu_cam0 +  0x0230), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS1_USER");
	PRINT_CLK((cmu_cam0 +  0x0234), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS1_USER");
	PRINT_CLK((cmu_cam0 +  0x0400), "CLK_CON_DIV_PCLK_CAM0_CSIS0_207");
	PRINT_CLK((cmu_cam0 +  0x040C), "CLK_CON_DIV_PCLK_CAM0_3AA0_207");
	PRINT_CLK((cmu_cam0 +  0x0410), "CLK_CON_DIV_PCLK_CAM0_3AA1_207");
	PRINT_CLK((cmu_cam0 +  0x0414), "CLK_CON_DIV_PCLK_CAM0_TREX_264");
	PRINT_CLK((cmu_cam0 +  0x0418), "CLK_CON_DIV_PCLK_CAM0_TREX_132");
	PRINT_CLK((cmu_cam0 +  0x0800), "CLK_ENABLE_ACLK_CAM0_CSIS0_414");
	PRINT_CLK((cmu_cam0 +  0x0804), "CLK_ENABLE_PCLK_CAM0_CSIS0_207");
	PRINT_CLK((cmu_cam0 +  0x080C), "CLK_ENABLE_ACLK_CAM0_CSIS1_168");
	PRINT_CLK((cmu_cam0 +  0x0818), "CLK_ENABLE_ACLK_CAM0_CSIS2_234");
	PRINT_CLK((cmu_cam0 +  0x081C), "CLK_ENABLE_ACLK_CAM0_CSIS3_132");
	PRINT_CLK((cmu_cam0 +  0x0828), "CLK_ENABLE_ACLK_CAM0_3AA0_414");
	PRINT_CLK((cmu_cam0 +  0x082C), "CLK_ENABLE_PCLK_CAM0_3AA0_207");
	PRINT_CLK((cmu_cam0 +  0x0830), "CLK_ENABLE_ACLK_CAM0_3AA1_414");
	PRINT_CLK((cmu_cam0 +  0x0834), "CLK_ENABLE_PCLK_CAM0_3AA1_207");
	PRINT_CLK((cmu_cam0 +  0x0838), "CLK_ENABLE_ACLK_CAM0_TREX_528");
	PRINT_CLK((cmu_cam0 +  0x083C), "CLK_ENABLE_PCLK_CAM0_TREX_264");
	PRINT_CLK((cmu_cam0 +  0x0840), "CLK_ENABLE_PCLK_CAM0_TREX_132");
	PRINT_CLK((cmu_cam0 +  0x0848), "CLK_ENABLE_PHYCLK_HS0_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x084C), "CLK_ENABLE_PHYCLK_HS1_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0850), "CLK_ENABLE_PHYCLK_HS2_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0854), "CLK_ENABLE_PHYCLK_HS3_CSIS0_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x0858), "CLK_ENABLE_PHYCLK_HS0_CSIS1_RX_BYTE");
	PRINT_CLK((cmu_cam0 +  0x085C), "CLK_ENABLE_PHYCLK_HS1_CSIS1_RX_BYTE");

	info("CAM1\n");
	PRINT_CLK((cmu_cam1 + 0x0200), "CLK_CON_MUX_ACLK_CAM1_ARM_672_USER");
	PRINT_CLK((cmu_cam1 + 0x0204), "CLK_CON_MUX_ACLK_CAM1_TREX_VRA_528_USER");
	PRINT_CLK((cmu_cam1 + 0x0208), "CLK_CON_MUX_ACLK_CAM1_TREX_B_528_USER");
	PRINT_CLK((cmu_cam1 + 0x020C), "CLK_CON_MUX_ACLK_CAM1_BUS_264_USER");
	PRINT_CLK((cmu_cam1 + 0x0210), "CLK_CON_MUX_ACLK_CAM1_PERI_84_USER");
	PRINT_CLK((cmu_cam1 + 0x0214), "CLK_CON_MUX_ACLK_CAM1_CSIS2_414_USER");
	PRINT_CLK((cmu_cam1 + 0x0218), "CLK_CON_MUX_ACLK_CAM1_CSIS3_132_USER");
	PRINT_CLK((cmu_cam1 + 0x021C), "CLK_CON_MUX_ACLK_CAM1_SCL_566_USER");
	PRINT_CLK((cmu_cam1 + 0x0220), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI0_USER");
	PRINT_CLK((cmu_cam1 + 0x0224), "CLK_CON_MUX_SCLK_CAM1_ISP_SPI1_USER");
	PRINT_CLK((cmu_cam1 + 0x0228), "CLK_CON_MUX_SCLK_CAM1_ISP_UART_USER");
	PRINT_CLK((cmu_cam1 + 0x022C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0230), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS1_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0234), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS2_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x0238), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS3_CSIS2_USER");
	PRINT_CLK((cmu_cam1 + 0x023C), "CLK_CON_MUX_PHYCLK_RXBYTECLKHS0_CSIS3_USER");
	PRINT_CLK((cmu_cam1 + 0x0400), "CLK_CON_DIV_PCLK_CAM1_ARM_168");
	PRINT_CLK((cmu_cam1 + 0x0408), "CLK_CON_DIV_PCLK_CAM1_TREX_VRA_264");
	PRINT_CLK((cmu_cam1 + 0x040C), "CLK_CON_DIV_PCLK_CAM1_BUS_132");
	PRINT_CLK((cmu_cam1 + 0x0800), "CLK_ENABLE_ACLK_CAM1_ARM_672");
	PRINT_CLK((cmu_cam1 + 0x0804), "CLK_ENABLE_PCLK_CAM1_ARM_168");
	PRINT_CLK((cmu_cam1 + 0x0808), "CLK_ENABLE_ACLK_CAM1_TREX_VRA_528");
	PRINT_CLK((cmu_cam1 + 0x080C), "CLK_ENABLE_PCLK_CAM1_TREX_VRA_264");
	PRINT_CLK((cmu_cam1 + 0x0810), "CLK_ENABLE_ACLK_CAM1_TREX_B_528");
	PRINT_CLK((cmu_cam1 + 0x0814), "CLK_ENABLE_ACLK_CAM1_BUS_264");
	PRINT_CLK((cmu_cam1 + 0x0818), "CLK_ENABLE_PCLK_CAM1_BUS_132");
	PRINT_CLK((cmu_cam1 + 0x081C), "CLK_ENABLE_PCLK_CAM1_PERI_84");
	PRINT_CLK((cmu_cam1 + 0x0820), "CLK_ENABLE_ACLK_CAM1_CSIS2_414");
	PRINT_CLK((cmu_cam1 + 0x0828), "CLK_ENABLE_ACLK_CAM1_CSIS3_132");
	PRINT_CLK((cmu_cam1 + 0x0830), "CLK_ENABLE_ACLK_CAM1_SCL_566");
	PRINT_CLK((cmu_cam1 + 0x083C), "CLK_ENABLE_PCLK_CAM1_SCL_283");
	PRINT_CLK((cmu_cam1 + 0x0840), "CLK_ENABLE_SCLK_CAM1_ISP_SPI0");
	PRINT_CLK((cmu_cam1 + 0x0844), "CLK_ENABLE_SCLK_CAM1_ISP_SPI1");
	PRINT_CLK((cmu_cam1 + 0x0848), "CLK_ENABLE_SCLK_CAM1_ISP_UART");
	PRINT_CLK((cmu_cam1 + 0x084C), "CLK_ENABLE_SCLK_ISP_PERI_IS_B");
	PRINT_CLK((cmu_cam1 + 0x0850), "CLK_ENABLE_PHYCLK_HS0_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0854), "CLK_ENABLE_PHYCLK_HS1_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0858), "CLK_ENABLE_PHYCLK_HS2_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x085C), "CLK_ENABLE_PHYCLK_HS3_CSIS2_RX_BYTE");
	PRINT_CLK((cmu_cam1 + 0x0860), "CLK_ENABLE_PHYCLK_HS0_CSIS3_RX_BYTE");

	info("ISP0\n");
	PRINT_CLK((cmu_isp0 + 0x0200), "CLK_CON_MUX_ACLK_ISP0_528_USER");
	PRINT_CLK((cmu_isp0 + 0x0204), "CLK_CON_MUX_ACLK_ISP0_TPU_400_USER");
	PRINT_CLK((cmu_isp0 + 0x0208), "CLK_CON_MUX_ACLK_ISP0_TREX_528_USER");
	PRINT_CLK((cmu_isp0 + 0x0400), "CLK_CON_DIV_PCLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0404), "CLK_CON_DIV_PCLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0408), "CLK_CON_DIV_PCLK_ISP0_TREX_264");
	PRINT_CLK((cmu_isp0 + 0x040C), "CLK_CON_DIV_PCLK_ISP0_TREX_132");
	PRINT_CLK((cmu_isp0 + 0x0800), "CLK_ENABLE_ACLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0808), "CLK_ENABLE_PCLK_ISP0");
	PRINT_CLK((cmu_isp0 + 0x080C), "CLK_ENABLE_ACLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0814), "CLK_ENABLE_PCLK_ISP0_TPU");
	PRINT_CLK((cmu_isp0 + 0x0818), "CLK_ENABLE_ACLK_ISP0_TREX");
	PRINT_CLK((cmu_isp0 + 0x081C), "CLK_ENABLE_PCLK_TREX_264");
	PRINT_CLK((cmu_isp0 + 0x0824), "CLK_ENABLE_PCLK_TREX_132");
	PRINT_CLK((cmu_isp0 + 0x0820), "CLK_ENABLE_PCLK_HPM_APBIF_ISP0");
	PRINT_CLK((cmu_isp0 + 0x0828), "CLK_ENABLE_SCLK_PROMISE_ISP0");

	info("ISP1\n");
	PRINT_CLK((cmu_isp1 + 0x0200), "CLK_CON_MUX_ACLK_ISP1_468_USER");
	PRINT_CLK((cmu_isp1 + 0x0400), "CLK_CON_DIV_PCLK_ISP1_234");
	PRINT_CLK((cmu_isp1 + 0x0800), "CLK_ENABLE_ACLK_ISP1");
	PRINT_CLK((cmu_isp1 + 0x0808), "CLK_ENABLE_PCLK_ISP1_234");
	PRINT_CLK((cmu_isp1 + 0x0810), "CLK_ENABLE_SCLK_PROMISE_ISP1");
}

static int exynos8890_fimc_is_print_clk(struct device *dev)
{
	info("#################### ISP0 clock ####################\n");
	fimc_is_get_rate(dev, "isp0");
	fimc_is_get_rate(dev, "isp0_tpu");
	fimc_is_get_rate(dev, "isp0_trex");
	fimc_is_get_rate(dev, "pxmxdx_isp0_pxl");

	info("#################### ISP1 clock ####################\n");
	fimc_is_get_rate(dev, "gate_fimc_isp1");
	fimc_is_get_rate(dev, "isp1");

	info("#################### SENSOR clock ####################\n");
	fimc_is_get_rate(dev, "isp_sensor0");
	fimc_is_get_rate(dev, "isp_sensor1");
	fimc_is_get_rate(dev, "isp_sensor2");
	fimc_is_get_rate(dev, "isp_sensor3");

	info("#################### CAM0 clock ####################\n");
	fimc_is_get_rate(dev, "gate_csis0");
	fimc_is_get_rate(dev, "gate_csis1");
	fimc_is_get_rate(dev, "gate_fimc_bns");
	fimc_is_get_rate(dev, "gate_fimc_3aa0");
	fimc_is_get_rate(dev, "gate_fimc_3aa1");
	fimc_is_get_rate(dev, "gate_hpm");
	fimc_is_get_rate(dev, "pxmxdx_csis0");
	fimc_is_get_rate(dev, "pxmxdx_csis1");
	fimc_is_get_rate(dev, "pxmxdx_csis2");
	fimc_is_get_rate(dev, "pxmxdx_csis3");
	fimc_is_get_rate(dev, "pxmxdx_3aa0");
	fimc_is_get_rate(dev, "pxmxdx_3aa1");
	fimc_is_get_rate(dev, "pxmxdx_trex");
	fimc_is_get_rate(dev, "hs0_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs1_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs2_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs3_csis0_rx_byte");
	fimc_is_get_rate(dev, "hs0_csis1_rx_byte");
	fimc_is_get_rate(dev, "hs1_csis1_rx_byte");

	info("#################### CAM1 clock ####################\n");
	fimc_is_get_rate(dev, "gate_isp_cpu");
	fimc_is_get_rate(dev, "gate_csis2");
	fimc_is_get_rate(dev, "gate_csis3");
	fimc_is_get_rate(dev, "gate_fimc_vra");
	fimc_is_get_rate(dev, "gate_mc_scaler");
	fimc_is_get_rate(dev, "gate_i2c0_isp");
	fimc_is_get_rate(dev, "gate_i2c1_isp");
	fimc_is_get_rate(dev, "gate_i2c2_isp");
	fimc_is_get_rate(dev, "gate_i2c3_isp");
	fimc_is_get_rate(dev, "gate_wdt_isp");
	fimc_is_get_rate(dev, "gate_mcuctl_isp");
	fimc_is_get_rate(dev, "gate_uart_isp");
	fimc_is_get_rate(dev, "gate_pdma_isp");
	fimc_is_get_rate(dev, "gate_pwm_isp");
	fimc_is_get_rate(dev, "gate_spi0_isp");
	fimc_is_get_rate(dev, "gate_spi1_isp");
	fimc_is_get_rate(dev, "isp_spi0");
	fimc_is_get_rate(dev, "isp_spi1");
	fimc_is_get_rate(dev, "isp_uart");
	fimc_is_get_rate(dev, "gate_sclk_pwm_isp");
	fimc_is_get_rate(dev, "gate_sclk_uart_isp");
	fimc_is_get_rate(dev, "cam1_arm");
	fimc_is_get_rate(dev, "cam1_vra");
	fimc_is_get_rate(dev, "cam1_trex");
	fimc_is_get_rate(dev, "cam1_bus");
	fimc_is_get_rate(dev, "cam1_peri");
	fimc_is_get_rate(dev, "cam1_csis2");
	fimc_is_get_rate(dev, "cam1_csis3");
	fimc_is_get_rate(dev, "cam1_scl");
	fimc_is_get_rate(dev, "cam1_phy0_csis2");
	fimc_is_get_rate(dev, "cam1_phy1_csis2");
	fimc_is_get_rate(dev, "cam1_phy2_csis2");
	fimc_is_get_rate(dev, "cam1_phy3_csis2");
	fimc_is_get_rate(dev, "cam1_phy0_csis3");

	exynos8890_fimc_is_print_clk_reg();

	return 0;
}

int exynos8890_fimc_is_clk_gate(u32 clk_gate_id, bool is_on)
{
	return 0;
}

int exynos8890_fimc_is_uart_gate(struct device *dev, bool mask)
{
	return 0;
}

int exynos8890_fimc_is_get_clk(struct device *dev)
{
	const char *name;
	struct clk *clk;
	u32 index;

	/* for debug */
	cmu_top = ioremap(0x10570000, SZ_4K);
	cmu_cam0 = ioremap(0x144D0000, SZ_4K);
	cmu_cam1 = ioremap(0x145D0000, SZ_4K);
	cmu_isp0 = ioremap(0x146D0000, SZ_4K);
	cmu_isp1 = ioremap(0x147D0000, SZ_4K);

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

int exynos8890_fimc_is_cfg_clk(struct device *dev)
{
	pr_debug("%s\n", __func__);

	/* Clock Gating */
	exynos8890_fimc_is_uart_gate(dev, false);

	return 0;
}

int exynos8890_fimc_is_clk_on(struct device *dev)
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

	/* CAM0 */
	//fimc_is_enable(dev, "gate_csis0");
	//fimc_is_enable(dev, "gate_csis1");
	fimc_is_enable(dev, "gate_fimc_bns");
	fimc_is_enable(dev, "gate_fimc_3aa0");
	fimc_is_enable(dev, "gate_fimc_3aa1");
	fimc_is_enable(dev, "gate_hpm");
	//fimc_is_enable(dev, "pxmxdx_csis0");
	//fimc_is_enable(dev, "pxmxdx_csis1");
	fimc_is_enable(dev, "pxmxdx_csis2");
	fimc_is_enable(dev, "pxmxdx_csis3");
	fimc_is_enable(dev, "pxmxdx_3aa0");
	fimc_is_enable(dev, "pxmxdx_3aa1");
	//fimc_is_enable(dev, "pxmxdx_trex");
	//fimc_is_enable(dev, "hs0_csis0_rx_byte");
	//fimc_is_enable(dev, "hs1_csis0_rx_byte");
	//fimc_is_enable(dev, "hs2_csis0_rx_byte");
	//fimc_is_enable(dev, "hs3_csis0_rx_byte");
	//fimc_is_enable(dev, "hs0_csis1_rx_byte");
	//fimc_is_enable(dev, "hs1_csis1_rx_byte");

	/* CAM1 */
	fimc_is_enable(dev, "gate_isp_cpu");
	//fimc_is_enable(dev, "gate_csis2");
	//fimc_is_enable(dev, "gate_csis3");
	fimc_is_enable(dev, "gate_fimc_vra");
	fimc_is_enable(dev, "gate_mc_scaler");
	fimc_is_enable(dev, "gate_i2c0_isp");
	fimc_is_enable(dev, "gate_i2c1_isp");
	fimc_is_enable(dev, "gate_i2c2_isp");
	fimc_is_enable(dev, "gate_i2c3_isp");
	fimc_is_enable(dev, "gate_wdt_isp");
	fimc_is_enable(dev, "gate_mcuctl_isp");
	fimc_is_enable(dev, "gate_uart_isp");
	fimc_is_enable(dev, "gate_pdma_isp");
	fimc_is_enable(dev, "gate_pwm_isp");
	fimc_is_enable(dev, "gate_spi0_isp");
	fimc_is_enable(dev, "gate_spi1_isp");
	fimc_is_enable(dev, "isp_spi0");
	fimc_is_enable(dev, "isp_spi1");
	fimc_is_enable(dev, "isp_uart");
	fimc_is_enable(dev, "gate_sclk_pwm_isp");
	fimc_is_enable(dev, "gate_sclk_uart_isp");
	fimc_is_enable(dev, "cam1_arm");
	fimc_is_enable(dev, "cam1_vra");
	//fimc_is_enable(dev, "cam1_trex");
	//fimc_is_enable(dev, "cam1_bus");
	fimc_is_enable(dev, "cam1_peri");
	//fimc_is_enable(dev, "cam1_csis2");
	//fimc_is_enable(dev, "cam1_csis3");
	fimc_is_enable(dev, "cam1_scl");
	//fimc_is_enable(dev, "cam1_phy0_csis2");
	//fimc_is_enable(dev, "cam1_phy1_csis2");
	//fimc_is_enable(dev, "cam1_phy2_csis2");
	//fimc_is_enable(dev, "cam1_phy3_csis2");
	//fimc_is_enable(dev, "cam1_phy0_csis3");

	fimc_is_set_rate(dev, "isp_spi0", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_spi1", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_uart", 132 * 1000000);

	/* ISP0 */
	fimc_is_enable(dev, "gate_fimc_isp0");
	fimc_is_enable(dev, "gate_fimc_tpu");
	fimc_is_enable(dev, "isp0");
	fimc_is_enable(dev, "isp0_tpu");
	fimc_is_enable(dev, "isp0_trex");
	fimc_is_enable(dev, "pxmxdx_isp0_pxl");

	/* ISP1 */
	fimc_is_enable(dev, "gate_fimc_isp1");
	fimc_is_enable(dev, "isp1");

#ifdef DBG_DUMPCMU
	exynos8890_fimc_is_print_clk(dev);
#endif

	pdata->clock_on = true;

p_err:
	return 0;
}

int exynos8890_fimc_is_clk_off(struct device *dev)
{
	int ret = 0;
	struct exynos_platform_fimc_is *pdata;

	pdata = dev_get_platdata(dev);
	if (!pdata->clock_on) {
		pr_err("clk_off is fail(already off)\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* CAM0 */
	//fimc_is_disable(dev, "gate_csis0");
	//fimc_is_disable(dev, "gate_csis1");
	fimc_is_disable(dev, "gate_fimc_bns");
	fimc_is_disable(dev, "gate_fimc_3aa0");
	fimc_is_disable(dev, "gate_fimc_3aa1");
	fimc_is_disable(dev, "gate_hpm");
	//fimc_is_disable(dev, "pxmxdx_csis0");
	//fimc_is_disable(dev, "pxmxdx_csis1");
	fimc_is_disable(dev, "pxmxdx_csis2");
	fimc_is_disable(dev, "pxmxdx_csis3");
	fimc_is_disable(dev, "pxmxdx_3aa0");
	fimc_is_disable(dev, "pxmxdx_3aa1");
	//fimc_is_disable(dev, "pxmxdx_trex");
	//fimc_is_disable(dev, "hs0_csis0_rx_byte");
	//fimc_is_disable(dev, "hs1_csis0_rx_byte");
	//fimc_is_disable(dev, "hs2_csis0_rx_byte");
	//fimc_is_disable(dev, "hs3_csis0_rx_byte");
	//fimc_is_disable(dev, "hs0_csis1_rx_byte");
	//fimc_is_disable(dev, "hs1_csis1_rx_byte");

	/* CAM1 */
	fimc_is_disable(dev, "gate_isp_cpu");
	//fimc_is_disable(dev, "gate_csis2");
	//fimc_is_disable(dev, "gate_csis3");
	fimc_is_disable(dev, "gate_fimc_vra");
	fimc_is_disable(dev, "gate_mc_scaler");
	fimc_is_disable(dev, "gate_i2c0_isp");
	fimc_is_disable(dev, "gate_i2c1_isp");
	fimc_is_disable(dev, "gate_i2c2_isp");
	fimc_is_disable(dev, "gate_i2c3_isp");
	fimc_is_disable(dev, "gate_wdt_isp");
	fimc_is_disable(dev, "gate_mcuctl_isp");
	fimc_is_disable(dev, "gate_uart_isp");
	fimc_is_disable(dev, "gate_pdma_isp");
	fimc_is_disable(dev, "gate_pwm_isp");
	fimc_is_disable(dev, "gate_spi0_isp");
	fimc_is_disable(dev, "gate_spi1_isp");
	fimc_is_disable(dev, "isp_spi0");
	fimc_is_disable(dev, "isp_spi1");
	fimc_is_disable(dev, "isp_uart");
	fimc_is_disable(dev, "gate_sclk_pwm_isp");
	fimc_is_disable(dev, "gate_sclk_uart_isp");
	fimc_is_disable(dev, "cam1_arm");
	fimc_is_disable(dev, "cam1_vra");
	//fimc_is_disable(dev, "cam1_trex");
	//fimc_is_disable(dev, "cam1_bus");
	fimc_is_disable(dev, "cam1_peri");
	//fimc_is_disable(dev, "cam1_csis2");
	//fimc_is_disable(dev, "cam1_csis3");
	fimc_is_disable(dev, "cam1_scl");
	//fimc_is_disable(dev, "cam1_phy0_csis2");
	//fimc_is_disable(dev, "cam1_phy1_csis2");
	//fimc_is_disable(dev, "cam1_phy2_csis2");
	//fimc_is_disable(dev, "cam1_phy3_csis2");
	//fimc_is_disable(dev, "cam1_phy0_csis3");

	/* ISP0 */
	fimc_is_disable(dev, "gate_fimc_isp0");
	fimc_is_disable(dev, "gate_fimc_tpu");
	fimc_is_disable(dev, "isp0");
	fimc_is_disable(dev, "isp0_tpu");
	fimc_is_disable(dev, "isp0_trex");
	fimc_is_disable(dev, "pxmxdx_isp0_pxl");

	/* ISP1 */
	fimc_is_disable(dev, "gate_fimc_isp1");
	fimc_is_disable(dev, "isp1");

	pdata->clock_on = false;

p_err:
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_clk_get(struct device *dev)
{
	exynos8890_fimc_is_get_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_cfg(struct device *dev)
{
	exynos8890_fimc_is_cfg_clk(dev);
	return 0;
}

int exynos_fimc_is_clk_on(struct device *dev)
{
	exynos8890_fimc_is_clk_on(dev);
	return 0;
}

int exynos_fimc_is_clk_off(struct device *dev)
{
	exynos8890_fimc_is_clk_off(dev);
	return 0;
}

int exynos_fimc_is_print_clk(struct device *dev)
{
	exynos8890_fimc_is_print_clk(dev);
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
	exynos8890_fimc_is_clk_gate(clk_gate_id, is_on);
	return 0;
}
