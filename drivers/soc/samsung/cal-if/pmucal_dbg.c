/*
 * Exynos PMUCAL debug interface support.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "pmucal_cpu.h"
#include "pmucal_local.h"
#include "pmucal_system.h"

#define PMUCAL_DBG_PREFIX	"PMUCAL-DBG"

static void __iomem *pmucal_dbg_latency_base;
static u32 pmucal_dbg_profile_en;
static u32 pmucal_dbg_profile_en_bit;
static u32 pmucal_dbg_profile_en_offset;
static struct dentry *pmucal_dbg_root;
static struct pmucal_dbg_info *pmucal_dbg_cpu_list;
static struct pmucal_dbg_info *pmucal_dbg_cluster_list;
static struct pmucal_dbg_info *pmucal_dbg_local_list;
static struct pmucal_dbg_info *pmucal_dbg_system_list;

void pmucal_dbg_set_emulation(struct pmucal_dbg_info *dbg)
{
	if (dbg->emul_en && !dbg->emul_enabled) {
		exynos_pmu_update(dbg->emul_offset, (1 << dbg->emul_bit), (1 << dbg->emul_bit));
		dbg->emul_enabled = 1;
	} else if (!dbg->emul_en && dbg->emul_enabled) {
		exynos_pmu_update(dbg->emul_offset, (1 << dbg->emul_bit), (0 << dbg->emul_bit));
		dbg->emul_enabled = 0;
	}
	return ;
}

void pmucal_dbg_req_emulation(struct pmucal_dbg_info *dbg, bool en)
{
	if (en) {
		dbg->emul_en++;
	} else {
		if (dbg->emul_en)
			dbg->emul_en--;
		else
			pr_err("%s: imbalanced %s.\n", PMUCAL_DBG_PREFIX, __func__);
	}

	return ;
}

void pmucal_dbg_do_profile(struct pmucal_dbg_info *dbg, bool is_on)
{
	u64 curr_latency;

	if (!pmucal_dbg_profile_en)
		return;

	if (is_on) {
		if (dbg->block_id == BLK_SYSTEM)
			curr_latency = (__raw_readl(pmucal_dbg_latency_base + dbg->latency_offset + 4) +
					__raw_readl(pmucal_dbg_latency_base + dbg->latency_offset + 4 + 8)) * 20;
		else
			curr_latency = __raw_readl(pmucal_dbg_latency_base + dbg->latency_offset + 4) * 20;
		if (!curr_latency)
			return ;

		if (dbg->on_latency_min > curr_latency)
			dbg->on_latency_min = curr_latency;
		dbg->on_latency_avg =
			(dbg->on_latency_avg * dbg->on_cnt + curr_latency) / (dbg->on_cnt + 1);
		if (dbg->on_latency_max < curr_latency)
			dbg->on_latency_max = curr_latency;

		dbg->on_cnt += 1;

	} else {
		if (dbg->block_id == BLK_SYSTEM)
			curr_latency = (__raw_readl(pmucal_dbg_latency_base + dbg->latency_offset) +
					__raw_readl(pmucal_dbg_latency_base + dbg->latency_offset + 8)) * 20;
		else
			curr_latency = __raw_readl(pmucal_dbg_latency_base + dbg->latency_offset) * 20;
		if (!curr_latency)
			return ;

		if (dbg->off_latency_min > curr_latency)
			dbg->off_latency_min = curr_latency;
		dbg->off_latency_avg =
			(dbg->off_latency_avg * dbg->off_cnt + curr_latency) / (dbg->off_cnt + 1);
		if (dbg->off_latency_max < curr_latency)
			dbg->off_latency_max = curr_latency;

		dbg->off_cnt += 1;
	}
}

static void pmucal_dbg_show_profile(struct pmucal_dbg_info *dbg)
{
	pr_info("min/avg/max on latency = %llu / %llu / %llu[nsec], count = %llu\n",
			dbg->on_latency_min, dbg->on_latency_avg,
			dbg->on_latency_max, dbg->on_cnt);
	pr_info("min/avg/max off latency = %llu / %llu / %llu[nsec], count = %llu\n",
			dbg->off_latency_min, dbg->off_latency_avg,
			dbg->off_latency_max, dbg->off_cnt);
}

static void pmucal_dbg_clear_profile(struct pmucal_dbg_info *dbg)
{
	dbg->on_latency_min = U32_MAX;
	dbg->on_latency_avg = 0;
	dbg->on_latency_max = 0;
	dbg->on_cnt = 0;
	dbg->off_latency_min = U32_MAX;
	dbg->off_latency_avg = 0;
	dbg->off_latency_max = 0;
	dbg->off_cnt = 0;
}

static ssize_t pmucal_dbg_emul_read(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct pmucal_dbg_info *data = file->private_data;
	char buf[80];
	ssize_t ret;

	ret = snprintf(buf, sizeof(buf), "emulation %s.\n", data->emul_en ? "enabled" : "disabled");
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t pmucal_dbg_emul_write(struct file *file, const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct pmucal_dbg_info *data = file->private_data;
	char buf[32];
	ssize_t len;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (len < 0)
		return len;

	buf[len] = '\0';

	switch (buf[0]) {
	case '0':
		pmucal_dbg_req_emulation(data, false);
		break;
	case '1':
		pmucal_dbg_req_emulation(data, true);
		break;
	default:
		pr_err("%s %s: Invalid input ['0'|'1']\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	return len;
}

static ssize_t pmucal_dbg_profile_read(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	pr_info("profile %s.\n",  pmucal_dbg_profile_en ? "enabled" : "disabled");

	return 0;
}

static ssize_t pmucal_dbg_profile_write(struct file *file, const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char buf[32];
	size_t buf_size;
	int i = 0;

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	switch (buf[0]) {
	case '0':
		exynos_pmu_update(pmucal_dbg_profile_en_offset,
				(1 << pmucal_dbg_profile_en_bit),
				(0 << pmucal_dbg_profile_en_bit));
		pmucal_dbg_profile_en = 0;

		pr_info("#####################################################\n");
		pr_info("######CPU######\n");
		for (i = 0; i < pmucal_cpu_list_size; i++) {
			if (pmucal_dbg_cpu_list[i].on_cnt == 0 || pmucal_dbg_cpu_list[i].off_cnt == 0)
				continue;
			pr_info("<Core%d>\n", pmucal_cpu_list[i].id);
			pmucal_dbg_show_profile(&pmucal_dbg_cpu_list[i]);
			pmucal_dbg_clear_profile(&pmucal_dbg_cpu_list[i]);
		}

		pr_info("######CLUSTER######\n");
		for (i = 0; i < pmucal_cluster_list_size; i++) {
			if (pmucal_dbg_cluster_list[i].on_cnt == 0 || pmucal_dbg_cluster_list[i].off_cnt == 0)
				continue;
			pr_info("<Cluster%d>\n", pmucal_cluster_list[i].id);
			pmucal_dbg_show_profile(&pmucal_dbg_cluster_list[i]);
			pmucal_dbg_clear_profile(&pmucal_dbg_cluster_list[i]);
		}

		pr_info("######Power Domain######\n");
		for (i = 0; i < pmucal_pd_list_size; i++) {
			if (pmucal_dbg_local_list[i].on_cnt == 0 || pmucal_dbg_local_list[i].off_cnt == 0)
				continue;
			pr_info("<%s>\n", pmucal_pd_list[i].name);
			pmucal_dbg_show_profile(&pmucal_dbg_local_list[i]);
			pmucal_dbg_clear_profile(&pmucal_dbg_local_list[i]);
		}

		pr_info("######System power mode######\n");
		for (i = 0; i < pmucal_lpm_list_size; i++) {
			if (pmucal_dbg_system_list[i].on_cnt == 0 || pmucal_dbg_system_list[i].off_cnt == 0)
				continue;
			pr_info("<Powermode%d>\n", pmucal_lpm_list[i].id);
			pmucal_dbg_show_profile(&pmucal_dbg_system_list[i]);
			pmucal_dbg_clear_profile(&pmucal_dbg_system_list[i]);
		}
		pr_info("#####################################################\n");
		break;
	case '1':
		exynos_pmu_update(pmucal_dbg_profile_en_offset,
				(1 << pmucal_dbg_profile_en_bit),
				(1 << pmucal_dbg_profile_en_bit));
		pmucal_dbg_profile_en = 1;
		break;
	default:
		pr_err("%s %s: Invalid input ['0'|'1']\n", PMUCAL_PREFIX, __func__);
		break;
	}

	return count;
}

static const struct file_operations pmucal_dbg_emul_fops = {
	.open = simple_open,
	.read = pmucal_dbg_emul_read,
	.write = pmucal_dbg_emul_write,
	.llseek = default_llseek,
};

static const struct file_operations pmucal_dbg_profile_fops = {
	.open = simple_open,
	.read = pmucal_dbg_profile_read,
	.write = pmucal_dbg_profile_write,
	.llseek = default_llseek,
};

int __init pmucal_dbg_init(void)
{
	struct device_node *node = NULL;
	int ret;
	u32 prop1, prop2, i;

	node = of_find_node_by_name(NULL, "pmucal_dbg");
	if (!node) {
		pr_err("%s %s:pmucal_dbg node has not been found.\n", PMUCAL_PREFIX, __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(node, "latency_base", &prop1);
	if (ret) {
		pr_err("%s %s:latency_base has not been found.\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "latency_size", &prop2);
	if (ret) {
		pr_err("%s %s:latency_size has not been found.\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	pmucal_dbg_latency_base = ioremap(prop1, prop2);
	if (pmucal_dbg_latency_base == NULL) {
		pr_err("%s %s:ioremap failed.\n", PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}

	ret = of_property_read_u32(node, "profile_en", &pmucal_dbg_profile_en);
	if (ret) {
		pr_err("%s %s:profile_en has not been found.\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "profile_en_offset", &pmucal_dbg_profile_en_offset);
	if (ret) {
		pr_err("%s %s:profile_en_offset has not been found.\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "profile_en_bit", &pmucal_dbg_profile_en_bit);
	if (ret) {
		pr_err("%s %s:profile_en_offset has not been found.\n", PMUCAL_PREFIX, __func__);
		return -EINVAL;
	}

	/* CPU */
	pmucal_dbg_cpu_list = kzalloc(sizeof(struct pmucal_dbg_info) * pmucal_cpu_list_size, GFP_KERNEL);
	if (!pmucal_dbg_cpu_list) {
		pr_err("%s %s:kzalloc failed on pmucal_dbg_cpu_list.\n", PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}
	while((node = of_find_node_by_type(node, "pmucal_dbg_cpu"))) {
		of_property_read_u32(node, "id", &i);
		pmucal_dbg_cpu_list[i].block_id = BLK_CPU;
		pmucal_dbg_cpu_list[i].pmucal_data = &pmucal_cpu_list[i];
		pmucal_cpu_list[i].dbg = &pmucal_dbg_cpu_list[i];

		of_property_read_u32(node, "emul_offset", &pmucal_dbg_cpu_list[i].emul_offset);
		of_property_read_u32(node, "emul_bit", &pmucal_dbg_cpu_list[i].emul_bit);
		of_property_read_u32(node, "emul_en", &pmucal_dbg_cpu_list[i].emul_en);
		pmucal_dbg_cpu_list[i].emul_enabled = 0;
		of_property_read_u32(node, "latency_offset", &pmucal_dbg_cpu_list[i].latency_offset);

		pmucal_dbg_clear_profile(&pmucal_dbg_cpu_list[i]);
	}

	/* Cluster */
	pmucal_dbg_cluster_list = kzalloc(sizeof(struct pmucal_dbg_info) * pmucal_cluster_list_size, GFP_KERNEL);
	if (!pmucal_dbg_cluster_list) {
		pr_err("%s %s:kzalloc failed on pmucal_dbg_cluster_list.\n", PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}
	while((node = of_find_node_by_type(node, "pmucal_dbg_cluster"))) {
		of_property_read_u32(node, "id", &i);
		pmucal_dbg_cluster_list[i].block_id = BLK_CLUSTER;
		pmucal_dbg_cluster_list[i].pmucal_data = &pmucal_cluster_list[i];
		pmucal_cluster_list[i].dbg = &pmucal_dbg_cluster_list[i];

		of_property_read_u32(node, "emul_offset", &pmucal_dbg_cluster_list[i].emul_offset);
		of_property_read_u32(node, "emul_bit", &pmucal_dbg_cluster_list[i].emul_bit);
		of_property_read_u32(node, "emul_en", &pmucal_dbg_cluster_list[i].emul_en);
		pmucal_dbg_cluster_list[i].emul_enabled = 0;
		of_property_read_u32(node, "latency_offset", &pmucal_dbg_cluster_list[i].latency_offset);

		pmucal_dbg_clear_profile(&pmucal_dbg_cluster_list[i]);
	}

	/* Local */
	pmucal_dbg_local_list = kzalloc(sizeof(struct pmucal_dbg_info) * pmucal_pd_list_size, GFP_KERNEL);
	if (!pmucal_dbg_local_list) {
		pr_err("%s %s:kzalloc failed on pmucal_dbg_local_list.\n", PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}
	while((node = of_find_node_by_type(node, "pmucal_dbg_local"))) {
		of_property_read_u32(node, "id", &i);
		pmucal_dbg_local_list[i].block_id = BLK_LOCAL;
		pmucal_dbg_local_list[i].pmucal_data = &pmucal_pd_list[i];
		pmucal_pd_list[i].dbg = &pmucal_dbg_local_list[i];

		of_property_read_u32(node, "emul_offset", &pmucal_dbg_local_list[i].emul_offset);
		of_property_read_u32(node, "emul_bit", &pmucal_dbg_local_list[i].emul_bit);
		of_property_read_u32(node, "emul_en", &pmucal_dbg_local_list[i].emul_en);
		pmucal_dbg_local_list[i].emul_enabled = 0;
		of_property_read_u32(node, "latency_offset", &pmucal_dbg_local_list[i].latency_offset);

		pmucal_dbg_clear_profile(&pmucal_dbg_local_list[i]);
	}

	/* System */
	pmucal_dbg_system_list = kzalloc(sizeof(struct pmucal_dbg_info) * pmucal_lpm_list_size, GFP_KERNEL);
	if (!pmucal_dbg_system_list) {
		pr_err("%s %s:kzalloc failed on pmucal_dbg_system_list.\n", PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}
	while((node = of_find_node_by_type(node, "pmucal_dbg_system"))) {
		of_property_read_u32(node, "id", &i);
		pmucal_dbg_system_list[i].block_id = BLK_SYSTEM;
		pmucal_dbg_system_list[i].pmucal_data = &pmucal_lpm_list[i];
		pmucal_lpm_list[i].dbg = &pmucal_dbg_system_list[i];

		of_property_read_u32(node, "emul_offset", &pmucal_dbg_system_list[i].emul_offset);
		of_property_read_u32(node, "emul_bit", &pmucal_dbg_system_list[i].emul_bit);
		of_property_read_u32(node, "emul_en", &pmucal_dbg_system_list[i].emul_en);
		pmucal_dbg_system_list[i].emul_enabled = 0;
		of_property_read_u32(node, "latency_offset", &pmucal_dbg_system_list[i].latency_offset);

		pmucal_dbg_clear_profile(&pmucal_dbg_system_list[i]);
	}

	return 0;
}

static int __init pmucal_dbg_debugfs_init(void)
{
	int i = 0;
	struct dentry *dentry;
	struct device_node *node = NULL;
	const char *buf;

	/* Common */
	pmucal_dbg_root = debugfs_create_dir("pmucal-dbg", NULL);
	if (!pmucal_dbg_root) {
		pr_err("%s %s: could not create debugfs dir\n",
				PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}

	dentry = debugfs_create_file("profile_en", 0644, pmucal_dbg_root,
					NULL, &pmucal_dbg_profile_fops);
	if (!dentry) {
		pr_err("%s %s: could not create profile_en file\n",
				PMUCAL_PREFIX, __func__);
		return -ENOMEM;
	}

	if (pmucal_dbg_profile_en) {
		exynos_pmu_update(pmucal_dbg_profile_en_offset,
				(1 << pmucal_dbg_profile_en_bit),
				(1 << pmucal_dbg_profile_en_bit));
	}

	/* CPU */
	while((node = of_find_node_by_type(node, "pmucal_dbg_cpu"))) {
		of_property_read_u32(node, "id", &i);
		of_property_read_string(node, "name", &buf);
		dentry = debugfs_create_dir(buf, pmucal_dbg_root);
		debugfs_create_file("emul_en", 0644, dentry, &pmucal_dbg_cpu_list[i],
					&pmucal_dbg_emul_fops);
	}

	/* Cluster */
	while((node = of_find_node_by_type(node, "pmucal_dbg_cluster"))) {
		of_property_read_u32(node, "id", &i);
		of_property_read_string(node, "name", &buf);
		dentry = debugfs_create_dir(buf, pmucal_dbg_root);
		debugfs_create_file("emul_en", 0644, dentry, &pmucal_dbg_cluster_list[i],
					&pmucal_dbg_emul_fops);
	}

	/* Local power domains */
	while((node = of_find_node_by_type(node, "pmucal_dbg_local"))) {
		of_property_read_u32(node, "id", &i);
		of_property_read_string(node, "name", &buf);
		dentry = debugfs_create_dir(buf, pmucal_dbg_root);
		debugfs_create_file("emul_en", 0644, dentry, &pmucal_dbg_local_list[i],
					&pmucal_dbg_emul_fops);
	}

	/* System power modes */
	while((node = of_find_node_by_type(node, "pmucal_dbg_system"))) {
		of_property_read_u32(node, "id", &i);
		of_property_read_string(node, "name", &buf);
		dentry = debugfs_create_dir(buf, pmucal_dbg_root);
		debugfs_create_file("emul_en", 0644, dentry, &pmucal_dbg_system_list[i],
					&pmucal_dbg_emul_fops);
	}

	return 0;
}
fs_initcall_sync(pmucal_dbg_debugfs_init);
