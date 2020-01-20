/* linux/drivers/soc/samsung/cal-if/pmucal_asv.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * ASV common driver for Exynos
 * Author: Hyunju Kang <hjtop.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fvmap_asv.h"
#include "pwrcal.h"
#include "vclk.h"
#include <soc/samsung/ect_parser.h>
#include "cmucal.h"

static void asv_set_grp(unsigned int id, unsigned int asvgrp)
{
	if (exynos_cal_asv_ops.set_grp)
		exynos_cal_asv_ops.set_grp(id, asvgrp);
}

static void asv_set_tablever(unsigned int version)
{
	asv_table_ver = version;
}

static void asv_set_ssa0(unsigned int id, unsigned int ssa0)
{
	if (exynos_cal_asv_ops.set_ssa0)
		exynos_cal_asv_ops.set_ssa0(id, ssa0);
}

static void asv_get_asvinfo(void)
{
	if (exynos_cal_asv_ops.asv_get_asvinfo)
		exynos_cal_asv_ops.asv_get_asvinfo();
}

static int get_asv_table(unsigned int *table, unsigned int id)
{
	int max_lv = 0;

	if (exynos_cal_asv_ops.get_asv_table)
		max_lv = exynos_cal_asv_ops.get_asv_table(table, id);

	return max_lv;
}

static int asv_rcc_set_table(void)
{
	int result = 0;

	if (exynos_cal_asv_ops.set_rcc_table)
		result = exynos_cal_asv_ops.set_rcc_table();

	return result;
}

static void asv_voltage_init_table(struct asv_table_list **asv_table, char *name)
{
	struct ect_voltage_domain *domain = NULL;
	struct ect_voltage_table *table = NULL;
	struct asv_table_entry *asv_entry = NULL;
	struct ect_margin_domain *margin_domain = NULL;
	void *asv_block = NULL, *margin_block = NULL;
	int i =0, j = 0, k = 0;

	asv_block = ect_get_block("ASV");
	if (asv_block == NULL)
		return;

	margin_block = ect_get_block("MARGIN");

	domain = ect_asv_get_domain(asv_block, name);
	if (domain == NULL)
		return;

	if (margin_block)
		margin_domain = ect_margin_get_domain(margin_block, name);

	*asv_table = kzalloc(sizeof(struct asv_table_list) * domain->num_of_table, GFP_KERNEL);
	if (*asv_table == NULL)
		return;

	for (i = 0; i < domain->num_of_table; ++i) {
		table = &domain->table_list[i];

		(*asv_table)[i].table_size = domain->num_of_table;
		(*asv_table)[i].table = kzalloc(sizeof(struct asv_table_entry) * domain->num_of_level, GFP_KERNEL);
		if ((*asv_table)[i].table == NULL)
			return;

		for (j = 0; j < domain->num_of_level; ++j) {
			asv_entry = &(*asv_table)[i].table[j];

			asv_entry->index = domain->level_list[j];
			asv_entry->voltage = kzalloc(sizeof(unsigned int) * domain->num_of_group, GFP_KERNEL);

			for (k = 0; k < domain->num_of_group; ++k) {
				if (table->voltages != NULL)
					asv_entry->voltage[k] = table->voltages[j * domain->num_of_group + k];
				else if (table->voltages_step != NULL)
					asv_entry->voltage[k] = table->voltages_step[j * domain->num_of_group + k] * table->volt_step;

				if (margin_domain != NULL) {
					if (margin_domain->offset != NULL)
						asv_entry->voltage[k] += margin_domain->offset[j * margin_domain->num_of_group + k];
					else
						asv_entry->voltage[k] += margin_domain->offset_compact[j * margin_domain->num_of_group + k] * margin_domain->volt_step;
				}
			}
		}
	}
}

static void asv_rcc_init_table(struct asv_table_list **rcc_table, char *name)
{
	struct ect_rcc_domain *domain = NULL;
	struct ect_rcc_table *table = NULL;
	struct asv_table_entry *rcc_entry = NULL;
	void *rcc_block = NULL;
	int i = 0, j = 0, k = 0;

	rcc_block = ect_get_block("RCC");
	if (rcc_block == NULL)
		return;

	domain = ect_rcc_get_domain(rcc_block, name);
	if (domain == NULL)
		return;

	*rcc_table = kzalloc(sizeof(struct asv_table_list) * domain->num_of_table, GFP_KERNEL);
	if (*rcc_table == NULL)
		return;

	for (i = 0; i < domain->num_of_table; ++i) {
		table = &domain->table_list[i];

		(*rcc_table)[i].table_size = domain->num_of_table;
		(*rcc_table)[i].table = kzalloc(sizeof(struct asv_table_entry) * domain->num_of_level, GFP_KERNEL);
		if ((*rcc_table)[i].table == NULL)
			return;

		for (j = 0; j < domain->num_of_level; ++j) {
			rcc_entry = &(*rcc_table)[i].table[j];

			rcc_entry->index = domain->level_list[j];
			rcc_entry->voltage = kzalloc(sizeof(unsigned int) * domain->num_of_group, GFP_KERNEL);

			for (k = 0; k < domain->num_of_group; ++k) {
				if (table->rcc != NULL)
					rcc_entry->voltage[k] = table->rcc[j * domain->num_of_group + k];
				else
					rcc_entry->voltage[k] = table->rcc_compact[j * domain->num_of_group + k];
			}
		}
	}
}

static void asv_voltage_table_init(void)
{
	int i = 0;

	for (i = 0; i < NUM_OF_DVFS; i++)
		asv_voltage_init_table(&(asv_table[i]), dvfs_names[i]);
}

static void asv_rcc_table_init(void)
{
	int i = 0;

	for (i = 0; i < NUM_OF_DVFS; i++)
		asv_rcc_init_table(&(rcc_table[i]), dvfs_names[i]);
}

static void asv_ssa_init(void)
{
	void *gen_block = 0;
	struct ect_gen_param_table *param = NULL;
	unsigned int asv_table_version = asv_table_ver;
	int i = 0, j = 0;

	gen_block = ect_get_block("GEN");
	if (gen_block == NULL)
		return;

	for (i = 0; i < NUM_OF_DVFS; i++) {
		param = ect_gen_param_get_table(gen_block, ssa_names[i]);
		if (param) {
			subgrp_table[i] = param->parameter[asv_table_version * param->num_of_col + SUB_GROUP_INDEX];
			ssa_info_table[i].ssa0_base = param->parameter[asv_table_version * param->num_of_col + SSA0_BASE_INDEX];
			ssa_info_table[i].ssa0_offset = param->parameter[asv_table_version * param->num_of_col + SSA0_OFFSET_INDEX];
			for (j = 0; j < size_of_ssa1_table; j++)
				ssa_info_table[i].ssa1_table[j] = param->parameter[asv_table_version * param->num_of_col + 4 + j];
		}
	}
}

static void asv_ema_init(void)
{
}

static void asv_print_info(void)
{
	int i = 0;

	pr_info("asv_table_ver : %d\n", asv_table_ver);
	pr_info("fused_grp : %d\n", fused_grp);

	for (i = 0; i < NUM_OF_DVFS; i++)
		pr_info("%s_asv_group : %u\n", dvfs_names[i], fused_table[i].asv_group);
}

static void rcc_print_info(void)
{
	if (exynos_cal_asv_ops.print_rcc_info)
		exynos_cal_asv_ops.print_rcc_info();
}

static int asv_set_ema(unsigned int id, unsigned int volt)
{
	int result = 0;

	if(exynos_cal_asv_ops.set_ema)
		result = exynos_cal_asv_ops.set_ema(id, volt);

	return result;
}

static int asv_get_grp(unsigned int id, unsigned int lv)
{
	int result = 0;

	if (exynos_cal_asv_ops.get_grp)
		result = exynos_cal_asv_ops.get_grp(id, lv);

	return result;
}

static int asv_get_tablever(void)
{
	return (int)(asv_table_ver);
}

int cal_asv_init(void)
{
	int i = 0;

	for (i = 0; i < NUM_OF_DVFS; i++) {
		asv_table[i] = NULL;
		rcc_table[i] = NULL;
	}

	asv_get_asvinfo();
	asv_voltage_table_init();
	asv_rcc_table_init();
	asv_ssa_init();
	asv_ema_init();

	if (exynos_cal_asv_ops.asv_init)
		exynos_cal_asv_ops.asv_init();

	return 0;
}

struct cal_asv_ops cal_asv_ops = {
	.print_asv_info = asv_print_info,
	.print_rcc_info = rcc_print_info,
	.asv_init = cal_asv_init,
	.set_grp = asv_set_grp,
	.get_grp = asv_get_grp,
	.get_asv_table = get_asv_table,
	.set_tablever = asv_set_tablever,
	.get_tablever = asv_get_tablever,
	.set_rcc_table = asv_rcc_set_table,
	.set_ssa0 = asv_set_ssa0,
	.set_ema = asv_set_ema,
};
