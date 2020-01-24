/**
 * phy-exynos-debug.h -  Samsung EXYNOS SoC series USB DRD PHY Debug Header
 *
 * Phy provider for USB 3.0 DRD controller on Exynos SoC series
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Author: Kyounghye Yunm <k-hye.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "phy-exynos-usbdrd.h"
#ifdef CONFIG_DEBUG_FS
extern int exynos_usbdrd_debugfs_init(struct exynos_usbdrd_phy *phy_drd);
extern int exynos_usbdrd_dp_debugfs_init(struct exynos_usbdrd_phy *phy_drd);
extern void exynos_usbdrd_debugfs_exit(struct exynos_usbdrd_phy *phy_drd);
#else
static inline int exynos_usbdrd_debugfs_init(struct exynos_usbdrd_phy *phy_drd)
{  return 0;  }
static inline void exynos_usbdrd_debugfs_exit(struct exynos_usbdrd_phy *phy_drd)
{  }
#endif

#define dump_register(nm)		\
{					\
	.name = __stringify(nm),	\
	.offset = EXYNOS_USBCON_##nm,	\
}
#define dump_regmap_mask(nm, bit_nm, bit)	\
{						\
	.name = __stringify(nm),		\
	.offset = EXYNOS_USBCON_##nm,		\
	.bitname = #bit_nm,			\
	.bitmask = nm##_##bit_nm##_MASK,	\
	.bitoffset = bit,			\
	.mask = true,				\
}
#define dump_regmap(nm, bit_nm, bit)	\
{					\
	.name = __stringify(nm),	\
	.offset = EXYNOS_USBCON_##nm,	\
	.bitname = #bit_nm,		\
	.bitmask = nm##_##bit_nm,	\
	.bitoffset = bit,		\
	.mask = false,			\
}
#define dump_register_dp(rnm)		\
{					\
	.name = __stringify(rnm),	\
	.offset = EXYNOS_USBDP_COM_##rnm,	\
}
#define dump_regmap_dp_mask(rnm, bit_rnm, bit_nm, bit)	\
{						\
	.name = __stringify(rnm),		\
	.offset = EXYNOS_USBDP_COM_##rnm,		\
	.bitname = #bit_nm,			\
	.bitmask = bit_rnm##_##bit_nm##_MASK,	\
	.bitoffset = bit,			\
	.mask = true,				\
}
#define dump_regmap_gen2_mask(rnm, bit_rnm, bit_nm, bit)	\
{						\
	.name = __stringify(rnm),		\
	.offset = EXYNOS_USBDP_COM_##rnm,		\
	.bitname = #bit_nm,			\
	.bitmask = bit_rnm##_##bit_nm##_MSK,	\
	.bitoffset = bit,			\
	.mask = true,				\
}
#define dump_regmap_dp(rnm, bit_rnm, bit_nm, bit)	\
{					\
	.name = __stringify(rnm),	\
	.offset = EXYNOS_USBDP_COM_##rnm,	\
	.bitname = #bit_nm,		\
	.bitmask = bit_rnm##_##bit_nm,	\
	.bitoffset = bit,		\
	.mask = false,			\
}

struct debugfs_regmap32 {
	char *name;
	char *bitname;
	unsigned int offset;
	unsigned int bitoffset;
	unsigned int bitmask;
	bool mask;
};

struct debugfs_regset_map {
	const struct debugfs_regmap32 *regs;
	int nregs;
};

struct exynos_debugfs_prvdata {
	struct exynos_usbdrd_phy *phy_drd;
	struct dentry	*root;
	struct debugfs_regset32 *regset;
	struct debugfs_regset_map *regmap;
};
