/* linux/drivers/soc/samsung/cal-if/fvmap_asv.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * ASV common header file for Exynos
 * Author: Hyunju Kang <hjtop.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FVMAP_ASV_H__
#define __FVMAP_ASV_H__

struct cal_asv_ops {
	void (*print_asv_info)(void);
	void (*print_rcc_info)(void);
	void (*set_grp)(unsigned int id, unsigned int asvgrp);
	int (*get_grp)(unsigned int id, unsigned int lv);
	void (*set_tablever)(unsigned int version);
	int (*get_tablever)(void);
	int (*set_rcc_table)(void);
	int (*asv_init)(void);
	unsigned int (*asv_pmic_info)(void);
	void (*set_ssa0)(unsigned int id, unsigned int ssa0);
	int (*get_asv_table)(unsigned int *table, unsigned int id);
	int (*set_ema)(unsigned int id, unsigned int volt);
};


struct exynos_cal_asv_ops {
	void (*asv_get_asvinfo)(void);
	void (*print_rcc_info)(void);
	int (*set_rcc_table)(void);
	void (*set_ssa0)(unsigned int id, unsigned int ssa0);
	int (*asv_init)(void);
	int (*get_asv_table)(unsigned int *table, unsigned int id);
	int (*get_grp)(unsigned int id, unsigned int lv);
	void (*set_grp)(unsigned int id, unsigned int asvgrp);
	int (*set_ema)(unsigned int id, unsigned int volt);
};

struct asv_table_entry {
	unsigned int index;
	unsigned int *voltage;
};

struct asv_table_list {
	struct asv_table_entry *table;
	unsigned int table_size;
	unsigned int voltage_count;
};

#define size_of_ssa1_table	8
struct ssa_info {
	unsigned int ssa0_base;
	unsigned int ssa0_offset;
	unsigned int ssa1_table[size_of_ssa1_table];
};

struct fused_info {
	unsigned int asv_group:4;
	int modified_group:4;
	unsigned int ssa10:2;
	unsigned int ssa11:2;
	unsigned int ssa0:4;
};

extern struct cal_asv_ops cal_asv_ops;
extern struct exynos_cal_asv_ops exynos_cal_asv_ops;
extern enum dvfs_id dvfs_id;
extern unsigned int NUM_OF_DVFS;
extern char *dvfs_names[];
extern char *ssa_names[];

extern struct asv_table_list *asv_table[];
extern struct asv_table_list *rcc_table[];
extern struct pwrcal_vclk_dfs *asv_dvfs_table[];
extern unsigned int subgrp_table[];
extern struct ssa_info ssa_info_table[];
extern struct fused_info fused_table[];

extern unsigned int force_asv_group[];

extern unsigned int asv_table_ver;
extern unsigned int fused_grp;

#define SUB_GROUP_INDEX		1
#define SSA0_BASE_INDEX		2
#define SSA0_OFFSET_INDEX	3

#define MAX_ASV_GROUP	16
#define NUM_OF_ASVTABLE	1
#define PWRCAL_ASV_LIST(table)	{table, sizeof(table) / sizeof(table[0])}

#define FORCE_ASV_MAGIC		0x57E90000

#endif
