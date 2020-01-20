/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file contains the utility functions for composite clocks.
 */

#ifndef __SAMSUNG_CLK_COMPOSITE_H
#define __SAMSUNG_CLK_COMPOSITE_H

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>

/*
 * struct samsung_clk_provider: information about clock provider
 * @reg_base: virtual address for the register base.
 * @clk_data: holds clock related data like clk* and number of clocks.
 * @lock: maintains exclusion bwtween callbacks for a given clock-provider.
 */
struct samsung_clk_provider {
	void __iomem *reg_base;
	struct clk_onecell_data clk_data;
	spinlock_t lock;
};

/*
 * struct samsung_clk_reg_dump: register dump of clock controller registers.
 * @offset: clock register offset from the controller base address.
 * @value: the value to be register at offset.
 */
struct samsung_clk_reg_dump {
	unsigned long	offset;
	u32		value;
};

/*
 * struct samsung_fixed_rate_clock: information about fixed-rate clock
 * @id: platform specific id of the clock.
 * @name: name of this fixed-rate clock.
 * @parent_name: optional parent clock name.
 * @flags: optional fixed-rate clock flags.
 * @fixed-rate: fixed clock rate of this clock.
 */
struct samsung_fixed_rate {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		fixed_rate;
};

#define FRATE(_id, cname, pname, f, frate)		\
	{						\
		.id		= _id,			\
		.name		= cname,		\
		.parent_name	= pname,		\
		.flags		= f,			\
		.fixed_rate	= frate,		\
	}

extern void __init samsung_register_fixed_rate(
		struct samsung_clk_provider *ctx,
		struct samsung_fixed_rate *list,
		unsigned int nr_clk);
/*
 * struct samsung_fixed_factor_clock: information about fixed-factor clock
 * @id: platform specific id of the clock.
 * @name: name of this fixed-factor clock.
 * @parent_name: parent clock name.
 * @mult: fixed multiplication factor.
 * @div: fixed division factor.
 * @flags: optional fixed-factor clock flags.
 */
struct samsung_fixed_factor {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		mult;
	unsigned long		div;
	unsigned long		flags;
};

#define FFACTOR(_id, cname, pname, m, d, f)		\
	{						\
		.id		= _id,			\
		.name		= cname,		\
		.parent_name	= pname,		\
		.mult		= m,			\
		.div		= d,			\
		.flags		= f,			\
	}

extern void __init samsung_register_fixed_factor(
		struct samsung_clk_provider *ctx,
		struct samsung_fixed_factor *list,
		unsigned int nr_clk);

/*
 * PLL145xx Clock Type: PLL1450x, PLL1451x, PLL1452x
 * Maximum lock time can be 150 * Pdiv cycles
 */
#define PLL145XX_LOCK_FACTOR		(150)
#define PLL145XX_MDIV_MASK		(0x3FF)
#define PLL145XX_PDIV_MASK		(0x03F)
#define PLL145XX_SDIV_MASK		(0x007)
#define PLL145XX_LOCKED_MASK		(0x1)
#define PLL145XX_MDIV_SHIFT		(16)
#define PLL145XX_PDIV_SHIFT		(8)
#define PLL145XX_SDIV_SHIFT		(0)
#define PLL145XX_LOCKED_SHIFT		(29)

/*
 * PLL1460X Clock Type
 * Maximum lock time can be 3000 * Pdiv cycles
 */
#define PLL1460X_LOCK_FACTOR		(3000)
#define PLL1460X_MDIV_MASK		(0x03FF)
#define PLL1460X_PDIV_MASK		(0x003F)
#define PLL1460X_SDIV_MASK		(0x0007)
#define PLL1460X_KDIV_MASK		(0xFFFF)
#define PLL1460X_LOCKED_MASK		(0x1)
#define PLL1460X_MDIV_SHIFT		(16)
#define PLL1460X_PDIV_SHIFT		(8)
#define PLL1460X_SDIV_SHIFT		(0)
#define PLL1460X_KDIV_SHIFT		(0)
#define PLL1460X_LOCKED_SHIFT		(29)

/*
 * PLL255xx Clock Type : PLL2551x, PLL2555x
 * Maximum lock time can be 200 * Pdiv cycles
 */
#define PLL255XX_LOCK_FACTOR		(200)
#define PLL255XX_MDIV_MASK		(0x3FF)
#define PLL255XX_PDIV_MASK		(0x3F)
#define PLL255XX_SDIV_MASK		(0x7)
#define PLL255XX_LOCKED_MASK		(0x1)
#define PLL255XX_MDIV_SHIFT		(12)
#define PLL255XX_PDIV_SHIFT		(4)
#define PLL255XX_SDIV_SHIFT		(0)
#define PLL255XX_LOCKED_SHIFT		(29)

/*
 * PLL2650X Clock Type
 * Maximum lock time can be 3000 * Pdiv cycles
 */
#define PLL2650X_LOCK_FACTOR		(3000)
#define PLL2650X_MDIV_MASK		(0x01FF)
#define PLL2650X_PDIV_MASK		(0x003F)
#define PLL2650X_SDIV_MASK		(0x0007)
#define PLL2650X_KDIV_MASK		(0xFFFF)
#define PLL2650X_LOCKED_MASK		(0x1)
#define PLL2650X_MDIV_SHIFT		(12)
#define PLL2650X_PDIV_SHIFT		(4)
#define PLL2650X_SDIV_SHIFT		(0)
#define PLL2650X_KDIV_SHIFT		(0)
#define PLL2650X_LOCKED_SHIFT		(29)

enum pll_type {
	pll_1450x = 0,
	pll_1451x,
	pll_1452x,
	pll_1460x,
	pll_2551x,
	pll_2555x,
	pll_2650x,
};

struct samsung_pll_rate_table {
	long rate;
	unsigned int pdiv;
	unsigned int mdiv;
	unsigned int sdiv;
	unsigned int kdiv;
};

#define PLL_BYPASS		BIT(0)
#define CHK_ON_CHANGING		BIT(7)
/*
 * struct samsung_composite_pll: information about composite-pll clocks
 * @id: id of the clock for binding with device tree.
 * @name: name of this pll clock.
 * @type: type of this pll clock.
 * @lock_reg: register for locking pll.
 * @con_reg: configuration register for pll.
 * @enable_reg: it can be different whether pll can be gated or only bypassed.
 * @enable_bit: bit index for en/disable pll.
 * @sel_reg: composite-pll has ctrl-mux. when disabled, it is set by 0.
 * @sel_bit: bit index for ctrl-mux.
 * @stat_reg: when sel_reg is set, status register must be checked.
 * @stat_bit: bit index for status register.
 * @rate_table: available pll output ratio table.
 * @alias: support alias for this clock.
 */
struct samsung_composite_pll {
	struct clk_hw				hw;
	unsigned int				id;
	const char				*name;
	enum pll_type				type;
	void __iomem				*lock_reg;
	void __iomem				*con_reg;
	void __iomem				*enable_reg;
	unsigned int				enable_bit;
	void __iomem				*sel_reg;
	unsigned int				sel_bit;
	void __iomem 				*stat_reg;
	unsigned int 				stat_bit;
	const struct samsung_pll_rate_table	*rate_table;
	unsigned int 				rate_count;
	unsigned int				pll_flag;
	u8					flag;
	const char				*alias;
};

#define PLL(_id, cname, _type, lock, con, en, enbit, sel, selbit, stat, statbit, rtable, pf, f, a)	\
	{											\
		.id		= _id,								\
		.name		= cname,							\
		.type		= _type,							\
		.lock_reg	= lock,								\
		.con_reg	= con,								\
		.enable_reg	= en,								\
		.enable_bit	= enbit,							\
		.sel_reg	= sel,								\
		.sel_bit	= selbit,							\
		.stat_reg	= stat,								\
		.stat_bit	= statbit,							\
		.rate_table	= rtable,							\
		.rate_count	= 0,								\
		.pll_flag	= pf,								\
		.flag		= f,								\
		.alias		= a,								\
	}

extern void samsung_register_comp_pll(
		struct samsung_clk_provider *ctx,
		struct samsung_composite_pll *pll_list,
		unsigned int nr_pll);

#define PNAME(x) static const char *x[]
/*
 * struct samsung_composite_mux: information about composite-mux clocks
 * @id: id of the clock for binding with device tree.
 * @name: name of this mux clock.
 * @parents: array of parent clocks.
 * @num_parents: number of parent clocks.
 * @sel_reg: register for mux selection.
 * @sel_bit: bit index for sel_reg.
 * @sel_width: different by number of parent clocks.
 * @stat_reg: status must be checked when changing parent.
 * @stat_bit: bit index for status register.
 * @stat_width: different by number of parent clocks.
 * @flag: optional flag for clock.
 * @alias: optional name. recommend no more than 15 characters.
 */
struct samsung_composite_mux {
	struct clk_hw			hw;
	unsigned int			id;
	const char			*name;
	const char			**parents;
	unsigned int			num_parents;
	void __iomem			*sel_reg;
	unsigned int			sel_bit;
	unsigned int			sel_width;
	void __iomem			*stat_reg;
	unsigned int 			stat_bit;
	unsigned int			stat_width;
	unsigned int			flag;
	const char			*alias;
	spinlock_t			*lock;
};

#define MUX(_id, cname, pnames, sel, selbit, selwid, stat, statbit, statwid, f, a)	\
	{										\
		.id		= _id,							\
		.name		= cname,						\
		.parents	= pnames,						\
		.num_parents	= ARRAY_SIZE(pnames),					\
		.sel_reg	= sel,							\
		.sel_bit	= selbit,						\
		.sel_width	= selwid,						\
		.stat_reg	= stat,							\
		.stat_bit	= statbit,						\
		.stat_width	= statwid,						\
		.flag		= f,							\
		.alias		= a,							\
	}

extern void samsung_register_comp_mux(
		struct samsung_clk_provider *ctx,
		struct samsung_composite_mux *mux_list,
		unsigned int nr_mux);

/*
 * struct samsung_composite_divider: information about composite-divider clocks
 * @id: id of the clock for binding with device tree.
 * @name: name of this divider clock.
 * @parent_name: name of parent clock.
 * @rate_reg: register for ratio selection.
 * @rate_bit: bit index for rate_reg.
 * @rate_width: can be different by bit index.
 * @stat_reg: status must be checked when changing ratio.
 * @stat_bit: bit index for status register.
 * @stat_width: can be different by bit index.
 * @flag: flag for clock.
 * @alias: optional name. recommend no more than 15 characters.
 */
struct samsung_composite_divider {
	struct clk_hw			hw;
	unsigned int			id;
	const char			*name;
	const char			*parent_name;
	void __iomem			*rate_reg;
	unsigned int			rate_bit;
	unsigned int			rate_width;
	void __iomem			*stat_reg;
	unsigned int			stat_bit;
	unsigned int			stat_width;
	unsigned int			flag;
	const char			*alias;
	spinlock_t			*lock;
};

#define DIV(_id, cname, pname, rate, ratebit, ratewid, stat, statbit, statwid, f, a)	\
	{										\
		.id		= _id,							\
		.name		= cname,						\
		.parent_name	= pname,						\
		.rate_reg	= rate,							\
		.rate_bit	= ratebit,						\
		.rate_width	= ratewid,						\
		.stat_reg	= stat,							\
		.stat_bit	= statbit,						\
		.stat_width	= statwid,						\
		.flag		= f,							\
		.alias		= a,							\
	}

extern void samsung_register_comp_divider(struct samsung_clk_provider *ctx,
		struct samsung_composite_divider *div_list, unsigned int nr_div);

#define CLK_GATE_ENABLE		BIT(20)
#define CLK_ON_CHANGING		BIT(7)
/*
 * struct samsung_gate: information about gate clocks
 * @id: id of the clock for binding with device tree.
 * @name:
 * @parent_name:
 * @reg:
 * @bit:
 * @flag:
 * @alias:
 */
struct samsung_gate {
	unsigned int			id;
	const char			*name;
	const char			*parent_name;
	void __iomem			*reg;
	u8				bit;
	unsigned int			flag;
	const char			*alias;
};

#define GATE(_id, cname, pname, r, b, f, a)		\
	{						\
		.id		= _id,			\
		.name		= cname,		\
		.parent_name	= pname,		\
		.reg		= r,			\
		.bit		= b,			\
		.flag		= f,			\
		.alias		= a,			\
	}

extern void __init samsung_register_gate(
		struct samsung_clk_provider *ctx,
		struct samsung_gate *gate_list,
		unsigned int nr_gate);

struct clk_samsung_usermux {
	struct clk_hw		hw;
	void __iomem		*sel_reg;
	u8			sel_bit;
	void __iomem		*stat_reg;
	u8	 		stat_bit;
	u8			flag;
	spinlock_t		*lock;
};

/*
 * struct samsung_composite_usermux: information about usermux clocks
 * @id: id of the clock for binding with device tree.
 * @name: name of this usermux clock.
 * @parent_name: name of parent clock.
 * @sel_reg: register for usermux selection.
 * @sel_bit: bit index for sel_reg.
 * @stat_reg: status must be checked when changing parent.
 * @stat_bit: bit index for status register.
 * @flag: optional flag for clock.
 * @alias: optional name. recommend no more than 15 characters.
 */
struct samsung_usermux {
	unsigned int			id;
	const char			*name;
	const char			*parent_name;
	void __iomem			*sel_reg;
	u8				sel_bit;
	void __iomem			*stat_reg;
	u8	 			stat_bit;
	u8				flag;
	const char			*alias;
};

#define USERMUX(_id, cname, pname, sel, selbit, stat, statbit, f, a)	\
	{								\
		.id		= _id,					\
		.name		= cname,				\
		.parent_name	= pname,				\
		.sel_reg	= sel,					\
		.sel_bit	= selbit,				\
		.stat_reg	= stat,					\
		.stat_bit	= statbit,				\
		.flag		= f,					\
		.alias		= a,					\
	}

extern void __init samsung_register_usermux(
		struct samsung_clk_provider *ctx,
		struct samsung_usermux *list,
		unsigned int nr_usermux);

#define VCLK_DFS		BIT(1)
#define VCLK_DFS_SWITCH		BIT(2)
#define VCLK_GATE		BIT(3)
#define VCLK_QCH_EN		BIT(4)
#define VCLK_QCH_DIS		BIT(5)
#define VCLK_QACTIVE		BIT(6)

/*
 * struct init_vclk: initial information for virtual clocks
 * @id: id of the clock for binding with device tree.
 * @calid: id of the clock for calling cal.
 * @name: name of this virtual clock.
 * @flags: optional flag for clock.
 * @vclk_flags: optional flag for only virtual clock.
 * @alias: optional name. recommend no more than 15 characters.
 */
struct init_vclk{
	unsigned int		id;
	unsigned int		calid;
	const char		*name;
	const char		*parent;
	u8			flags;
	u8			vclk_flags;
	const char		*alias;
	u32			addr;
	u32			mask;
	u32			val;
};

struct samsung_vclk {
	struct clk_hw		hw;
	unsigned int		id;
	u8			flags;
	spinlock_t		*lock;
	void __iomem		*addr;
	u32			mask;
	u32			val;
};

#define VCLK(_id, _calid, _name, f, vf, a)	\
	{					\
		.id		= _id,		\
		.calid		= _calid,	\
		.name		= _name,	\
		.flags		= f,		\
		.vclk_flags	= vf,		\
		.alias		= a,		\
	}

#define HWACG_VCLK(_id, _calid, _name, _parent, f, vf, a)	\
	{					\
		.id		= _id,		\
		.calid		= _calid,	\
		.name		= _name,	\
		.parent		= _parent,	\
		.flags		= f,		\
		.vclk_flags	= vf,		\
		.alias		= a,		\
	}

#define HWACG_VCLK_QACTIVE(_id, _name, _parent, f, vf, a, _addr, _mask, _val)	\
	{					\
		.id		= _id,		\
		.name		= _name,	\
		.parent		= _parent,	\
		.flags		= f,		\
		.vclk_flags	= vf,		\
		.alias		= a,		\
		.addr		= _addr,	\
		.mask		= _mask,	\
		.val		= _val,		\
	}

extern void __init samsung_register_vclk(struct samsung_clk_provider *ctx,
		struct init_vclk *list, unsigned int nr_vclk);

extern struct samsung_clk_provider *__init samsung_clk_init(
			struct device_node *np, void __iomem *base,
			unsigned long nr_clks);

extern void __init samsung_clk_of_add_provider(struct device_node *np,
		struct samsung_clk_provider *ctx);

extern void __init samsung_register_of_fixed_ext(
			struct samsung_clk_provider *ctx,
			struct samsung_fixed_rate *fixed_rate_clk,
			unsigned int nr_fixed_rate_clk,
			struct of_device_id *clk_matches);

extern void samsung_clk_save(void __iomem *base,
			struct samsung_clk_reg_dump *rd,
			unsigned int num_regs);
extern void samsung_clk_restore(void __iomem *base,
			const struct samsung_clk_reg_dump *rd,
			unsigned int num_regs);
extern struct samsung_clk_reg_dump *samsung_clk_alloc_reg_dump(
			const unsigned long *rdump,
			unsigned long nr_rdump);

#endif /* __SAMSUNG_CLK_COMPOSITE_H */
