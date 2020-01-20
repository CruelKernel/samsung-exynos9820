/* linux/arch/arm/plat-s5p/include/plat/fimc_is.h
 *
 * Copyright (C) 2011 Samsung Electronics, Co. Ltd
 *
 * Exynos 4 series FIMC-IS slave device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef EXYNOS_FIMC_IS_H_
#define EXYNOS_FIMC_IS_H_

#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include "fimc-is-hw-dvfs.h"

extern int debug_clk;

struct fimc_is_clk {
	const char *name;
	struct clk *clk;
};

enum FIMC_IS_DVFS_QOS_TYPE {
	FIMC_IS_DVFS_CPU_MIN,
	FIMC_IS_DVFS_CPU_MAX,
	FIMC_IS_DVFS_INT_CAM,
	FIMC_IS_DVFS_INT,
	FIMC_IS_DVFS_MIF,
	FIMC_IS_DVFS_I2C,
	FIMC_IS_DVFS_CAM,
	FIMC_IS_DVFS_DISP,
	FIMC_IS_DVFS_HPG,
	FIMC_IS_DVFS_END,
};

enum FIMC_IS_CLK_GATE {
	FIMC_IS_GATE_3AA1_IP,
	FIMC_IS_GATE_ISP_IP,
	FIMC_IS_GATE_DRC_IP,
	FIMC_IS_GATE_SCC_IP,
	FIMC_IS_GATE_ODC_IP,
	FIMC_IS_GATE_DIS_IP,
	FIMC_IS_GATE_3DNR_IP,
	FIMC_IS_GATE_SCP_IP,
	FIMC_IS_GATE_FD_IP,
	FIMC_IS_GATE_3AA0_IP,
	FIMC_IS_GATE_ISP1_IP,
	FIMC_IS_GATE_TPU_IP,
	FIMC_IS_GATE_VRA_IP,
	FIMC_IS_CLK_GATE_MAX,
};

enum FIMC_IS_GRP {
	FIMC_IS_GRP_3A0,
	FIMC_IS_GRP_3A1,
	FIMC_IS_GRP_ISP,
	FIMC_IS_GRP_DIS,
	FIMC_IS_GRP_MAX,
};

enum FIMC_IS_CLK_GATE_USR_SCENARIO {
	CLK_GATE_NOT_FULL_BYPASS_SN = 1,
	CLK_GATE_FULL_BYPASS_SN,
	CLK_GATE_DIS_SN,
};

/*
 * struct exynos_fimc_is_clk_gate_group
 *	This struct is for host clock gating.
 * 	It decsribes register, masking bit info and other control for each group.
 *	If you uses host clock gating, You must define this struct in exynos_fimc_is_clk_gate_info.
 */
struct exynos_fimc_is_clk_gate_group {
	u32	mask_clk_on_org;	/* masking value in clk on */
	u32	mask_clk_on_mod;	/* masking value in clk on */
	u32	mask_clk_off_self_org;	/* masking value in clk off(not depend) original */
	u32	mask_clk_off_self_mod;	/* masking value in clk off(not depend) final */
	u32	mask_clk_off_depend;	/* masking value in clk off(depend on other grp) */
	u32	mask_cond_for_depend;	/* masking group having dependancy for other */
};

/*
 * struct exynos_fimc_is_clk_gate_info
 * 	This struct is for host clock gating.
 * 	It has exynos_fimc_is_clk_gate_group to control each group's clk gating.
 * 	And it has function pointer to include user scenario masking
 */
struct exynos_fimc_is_clk_gate_info {
	const char *gate_str[FIMC_IS_CLK_GATE_MAX];			/* register adr for gating */
	struct exynos_fimc_is_clk_gate_group groups[FIMC_IS_GRP_MAX];
	/* You must set this function pointer (on/off) */
	int (*clk_on_off)(u32 clk_gate_id, bool is_on);
	/*
	 * if there are specific scenarios for clock gating,
	 * You can define user function.
	 * user_scenario_id will be in
	 */
	int (*user_clk_gate)(u32 group_id,
			bool is_on,
			u32 user_scenario_id,
			unsigned long msk_state,
			struct exynos_fimc_is_clk_gate_info *gate_info);
};

/**
* struct exynos_platform_fimc_is - camera host interface platform data
*
* @isp_info: properties of camera sensor required for host interface setup
*/
struct exynos_platform_fimc_is {
	int hw_ver;
	bool clock_on;
	int (*clk_get)(struct device *dev);
	int (*clk_cfg)(struct device *dev);
	int (*clk_on)(struct device *dev);
	int (*clk_off)(struct device *dev);
	int (*print_clk)(struct device *dev);
	int (*print_cfg)(struct device *dev, u32 channel);
	struct pinctrl *pinctrl;
	/* These fields are to return qos value for dvfs scenario */
	u32 dvfs_data[FIMC_IS_DVFS_TABLE_IDX_MAX][FIMC_IS_SN_END][FIMC_IS_DVFS_END];

	/* For host clock gating */
	struct exynos_fimc_is_clk_gate_info *gate_info;
};

extern struct device *fimc_is_dev;

int fimc_is_set_parent_dt(struct device *dev, const char *child, const char *parent);
int fimc_is_set_rate_dt(struct device *dev, const char *conid, unsigned int rate);
ulong fimc_is_get_rate_dt(struct device *dev, const char *conid);
int fimc_is_enable_dt(struct device *dev, const char *conid);
int fimc_is_disable_dt(struct device *dev, const char *conid);

int fimc_is_set_rate(struct device *dev, const char *name, ulong frequency);
ulong fimc_is_get_rate(struct device *dev, const char *conid);
int fimc_is_enable(struct device *dev, const char *conid);
int fimc_is_disable(struct device *dev, const char *conid);
#ifdef CONFIG_SOC_EXYNOS9820
int is_enabled_clk_disable(struct device *dev, const char *conid);
#endif

/* platform specific clock functions */
int exynos_fimc_is_clk_get(struct device *dev);
int exynos_fimc_is_clk_cfg(struct device *dev);
int exynos_fimc_is_clk_on(struct device *dev);
int exynos_fimc_is_clk_off(struct device *dev);
int exynos_fimc_is_print_clk(struct device *dev);
int exynos_fimc_is_clk_gate(u32 clk_gate_id, bool is_on);

#endif /* EXYNOS_FIMC_IS_H_ */
