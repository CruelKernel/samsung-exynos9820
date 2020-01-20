/*
 * Exynos regulator support.
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
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
#include <linux/mutex.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include "../../regulator/internal.h"
#include <../drivers/soc/samsung/acpm/acpm_ipc.h>

#define EXYNOS_RGT_PREFIX	"EXYNOS-RGT: "

struct exynos_rgt_info {
	struct regulator *rgt;
	struct dentry *reg_dir;
	struct dentry *f_get;
	struct dentry *f_ena;
	struct dentry *f_volt;
	struct file_operations get_fops;
	struct file_operations ena_fops;
	struct file_operations volt_fops;
};

static struct dentry *exynos_rgt_root;
static int num_regulators = 0;

#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
#define REG_MAP_MAX		(0x100)

struct regulator_ss_info {
	char name[7];
	unsigned int min_uV;
	unsigned int uV_step;
	unsigned int linear_min_sel;
	unsigned int vsel_reg;
};

struct exynos_rgt_ss_info {
	u32 acpm_pmic_cnt;
	u32 *acpm_reg_cnt;
	struct regulator_ss_info **regulator_ss;
	char **reg_map;
	bool is_reg_map_set;
};
struct exynos_rgt_ss_info rgt_ss_info;
#endif

static const char *rdev_get_name(struct regulator_dev *rdev)
{
	if (rdev->desc->name)
		return rdev->desc->name;
	else if (rdev->constraints && rdev->constraints->name)
		return rdev->constraints->name;
	else
		return "";
}

static ssize_t exynos_rgt_get_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct regulator *cons, *rgt = file->private_data;
	const char *dev;
	char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	ssize_t len, ret = 0;

	if (!buf)
		return -ENOMEM;

	len = snprintf(buf + ret, PAGE_SIZE - ret, "[%s]\t open_count %d\n",
			rdev_get_name(rgt->rdev),
			rgt->rdev->open_count);
	if (len > 0)
		ret += len;

	len = snprintf(buf + ret, PAGE_SIZE - ret, "consumer list ->\n");
	if (len > 0)
		ret += len;

	if (list_empty(&rgt->rdev->consumer_list))
		goto skip;

	list_for_each_entry(cons, &rgt->rdev->consumer_list, list) {
		if (cons && cons->dev && dev_name(cons->dev))
			dev = dev_name(cons->dev);
		else
			dev = "unknown";

		len = snprintf(buf + ret, PAGE_SIZE - ret, "\t [%s]\n", dev);
		if (len > 0)
			ret += len;

		if (ret > PAGE_SIZE) {
			ret = PAGE_SIZE;
			break;
		}
	}
skip:
	ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

	kfree(buf);
	return ret;
}

static ssize_t exynos_rgt_ena_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct regulator *rgt = file->private_data;
	char buf[80];
	ssize_t ret;

	if (!rgt->rdev->desc->ops->is_enabled) {
		pr_err("There is no is_enabled callback on this regulator\n");
		return -ENODEV;
	}

	ret = snprintf(buf, sizeof(buf), "[%s]\t %s (always_on %d, use_count %d)\n",
			rdev_get_name(rgt->rdev),
			rgt->rdev->desc->ops->is_enabled(rgt->rdev) ? "enabled " : "disabled",
			rgt->rdev->constraints->always_on,
			rgt->rdev->use_count);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t exynos_rgt_ena_write(struct file *file, const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct regulator *rgt = file->private_data;
	char buf[32];
	ssize_t len, ret;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (len < 0)
		return len;

	buf[len] = '\0';

	switch (buf[0]) {
	case '0':
		ret = regulator_disable(rgt);
		if (ret)
			return ret;
		break;
	case '1':
		ret = regulator_enable(rgt);
		if (ret)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return len;
}

static ssize_t exynos_rgt_volt_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct regulator *cons, *rgt = file->private_data;
	struct regulation_constraints *constraints = rgt->rdev->constraints;
	const char *dev;
	char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	ssize_t len, ret = 0;

	if (!buf)
		return -ENOMEM;

	len = snprintf(buf + ret, PAGE_SIZE - ret, "[%s]\t curr %4d mV\t constraint min %4d mV, max %4d mV\n",
			rdev_get_name(rgt->rdev),
			regulator_get_voltage(rgt) / 1000,
			constraints->min_uV / 1000,
			constraints->max_uV / 1000);
	if (len > 0)
		ret += len;

	len = snprintf(buf + ret, PAGE_SIZE - ret, "consumer list ->\n");
	if (len > 0)
		ret += len;

	if (list_empty(&rgt->rdev->consumer_list))
		goto skip;

	list_for_each_entry(cons, &rgt->rdev->consumer_list, list) {
		if (cons && cons->dev && dev_name(cons->dev))
			dev = dev_name(cons->dev);
		else
			dev = "unknown";

		len = snprintf(buf + ret, PAGE_SIZE - ret,
				"\t [%s]\t min %4d mV, max %4d mV %s\n",
				dev,
				cons->min_uV / 1000,
				cons->max_uV / 1000,
				cons->min_uV ? "(requested)" : "");
		if (len > 0)
			ret += len;

		if (ret > PAGE_SIZE) {
			ret = PAGE_SIZE;
			break;
		}
	}
skip:
	ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

	kfree(buf);
	return ret;
}

static ssize_t exynos_rgt_volt_write(struct file *file, const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct regulator *rgt = file->private_data;
	int min_mV, min_uV, max_uV = rgt->rdev->constraints->max_uV;
	char buf[32];
	ssize_t len, ret;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (len < 0)
		return len;

	buf[len] = '\0';

	ret = kstrtos32(buf, 10, &min_mV);
	if (ret)
		return ret;

	min_uV = min_mV * 1000;

	if (min_uV < rgt->rdev->constraints->min_uV || min_uV > max_uV)
		return -EINVAL;

	ret = regulator_set_voltage(rgt, min_uV, max_uV);
	if (ret)
		return ret;

	return len;
}

static const struct file_operations exynos_rgt_get_fops = {
	.open = simple_open,
	.read = exynos_rgt_get_read,
	.llseek = default_llseek,
};

static const struct file_operations exynos_rgt_ena_fops = {
	.open = simple_open,
	.read = exynos_rgt_ena_read,
	.write = exynos_rgt_ena_write,
	.llseek = default_llseek,
};

static const struct file_operations exynos_rgt_volt_fops = {
	.open = simple_open,
	.read = exynos_rgt_volt_read,
	.write = exynos_rgt_volt_write,
	.llseek = default_llseek,
};

#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
struct regulator_ss_info *get_regulator_ss(u32 rgt_idx, u32 spd_ch)
{
	if (spd_ch < rgt_ss_info.acpm_pmic_cnt && rgt_idx < rgt_ss_info.acpm_reg_cnt[spd_ch])
		return &(rgt_ss_info.regulator_ss[spd_ch][rgt_idx]);
	else
		return NULL;
}

static void set_reg_map(void)
{
	u32 idx;
	int i, j;

	for (i = 0; i < rgt_ss_info.acpm_pmic_cnt; i++) {
		for (j = 0; j < rgt_ss_info.acpm_reg_cnt[i]; j++) {
			idx = rgt_ss_info.regulator_ss[i][j].vsel_reg & 0xFF;
			if (idx == 0)
				continue;

			if ((rgt_ss_info.reg_map[i][idx]) != 0)
				pr_err("duplicated set_reg_map [%d] reg_map %x\n", j,
						rgt_ss_info.reg_map[i][idx]);

			rgt_ss_info.reg_map[i][idx] = j;
		}
	}

	rgt_ss_info.is_reg_map_set = true;
}

static u32 get_reg_id(u32 ch, u32 addr)
{
	u32 id;

	if (rgt_ss_info.is_reg_map_set == false)
		return REG_MAP_MAX;

	if (addr >> 8 != 0x1 || ch >= rgt_ss_info.acpm_pmic_cnt)
		return REG_MAP_MAX;

	id = rgt_ss_info.reg_map[ch][addr & 0xff];
	if (id != 0)
		return id;

	return REG_MAP_MAX;
}

static u32 get_reg_voltage(struct regulator_ss_info reg_info, u32 selector)
{
	if (reg_info.name[0] == 'L')
		selector = selector & 0x3F;

	return reg_info.min_uV + (reg_info.uV_step * (selector - reg_info.linear_min_sel));
}

static int prepare_dss_regulator(struct platform_device *pdev)
{
	struct device_node *exynos_rgt_np = pdev->dev.of_node;
	struct device_node *regulators_np, *pmic_np;
	int ret, ipc_ch, spd_ch, i;

	ret = of_property_read_u32(exynos_rgt_np, "num-acpm-pmic", &(rgt_ss_info.acpm_pmic_cnt));
	if (ret) {
		pr_err("%s parse fail on num-acpm-pmic\n", EXYNOS_RGT_PREFIX);
		return -ENODEV;
	}

	rgt_ss_info.acpm_reg_cnt = kzalloc(sizeof(u32) * rgt_ss_info.acpm_pmic_cnt, GFP_KERNEL);
	if (!rgt_ss_info.acpm_reg_cnt) {
		pr_err("%s mem alloc fail on acpm_reg_cnt.\n", EXYNOS_RGT_PREFIX);
		return -ENOMEM;
	}

	regulators_np = of_find_node_by_name(NULL, "regulators");
	while (regulators_np) {
		pmic_np = of_get_parent(regulators_np);
		ret = of_property_read_u32(pmic_np, "acpm-ipc-channel", &ipc_ch);
		if (!ret) {
			ret = of_property_read_u32(pmic_np, "acpm-speedy-channel", &spd_ch);
			if (!ret) {
				if (spd_ch < rgt_ss_info.acpm_pmic_cnt) {
					rgt_ss_info.acpm_reg_cnt[spd_ch] = of_get_child_count(regulators_np);
				}
			}
		}
		regulators_np = of_find_node_by_name(regulators_np, "regulators");
	}

	rgt_ss_info.regulator_ss = kzalloc(sizeof(struct regulator_ss_info *) * rgt_ss_info.acpm_pmic_cnt, GFP_KERNEL);
	if (!(rgt_ss_info.regulator_ss)) {
		pr_err("%s mem alloc fail on *regulator_ss.\n", EXYNOS_RGT_PREFIX);
		return -ENOMEM;
	}
	for (i = 0; i < rgt_ss_info.acpm_pmic_cnt; i++) {
		rgt_ss_info.regulator_ss[i] = kzalloc(sizeof(struct regulator_ss_info) * rgt_ss_info.acpm_reg_cnt[i], GFP_KERNEL);
		if (!(rgt_ss_info.regulator_ss[i])) {
			pr_err("%s mem alloc fail on regulator_ss.\n", EXYNOS_RGT_PREFIX);
			return -ENOMEM;
		}
	}

	rgt_ss_info.reg_map = kzalloc(sizeof(char *) * rgt_ss_info.acpm_pmic_cnt, GFP_KERNEL);
	if (!(rgt_ss_info.reg_map)) {
		pr_err("%s mem alloc fail on *reg_map.\n", EXYNOS_RGT_PREFIX);
		return -ENOMEM;
	}
	for (i = 0; i < rgt_ss_info.acpm_pmic_cnt; i++) {
		rgt_ss_info.reg_map[i] = kzalloc(sizeof(char) * REG_MAP_MAX, GFP_KERNEL);
		if (!(rgt_ss_info.reg_map[i])) {
			pr_err("%s mem alloc fail on reg_map.\n", EXYNOS_RGT_PREFIX);
			return -ENOMEM;
		}
	}

	return 0;
}
#endif

void exynos_rgt_dbg_snapshot_regulator(u32 val, unsigned long long time)
{
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	u32 spd_ch, reg_addr, mod_addr, reg_data, reg_id;

	spd_ch = (val >> 28) & 0xf;
	reg_addr = (val >> 12) & 0xfff;
	reg_data = (val >> 4) & 0xff;
	mod_addr = (spd_ch << 12) | reg_addr;
	reg_id = get_reg_id(spd_ch, reg_addr);

	if (reg_id == REG_MAP_MAX)
		dbg_snapshot_regulator(time, "CHKADDR", mod_addr, reg_data, reg_data, val & 0xF);
	else
		dbg_snapshot_regulator(time, rgt_ss_info.regulator_ss[spd_ch][reg_id].name, mod_addr,
					get_reg_voltage(rgt_ss_info.regulator_ss[spd_ch][reg_id], reg_data),
					reg_data, val & 0xf);
#endif
	return ;
}

static int exynos_rgt_probe(struct platform_device *pdev)
{
	int ret;
	struct exynos_rgt_info *rgt_info;
	struct device_node *regulators_np, *reg_np;
	int rgt_idx = 0;
	const char *rgt_name;
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	struct regulator_desc rdesc;
	int rgt_ss_idx, ipc_ch, spd_ch;
	struct device_node *pmic_np;
	struct regulator_ss_info *rgt_ss;
#endif

	regulators_np = of_find_node_by_name(NULL, "regulators");
	if (!regulators_np) {
		pr_err("%s %s: could not find regulators sub-node\n", EXYNOS_RGT_PREFIX, __func__);
		ret = -EINVAL;
		goto err_find_regs;
	}

	while (regulators_np) {
		num_regulators += of_get_child_count(regulators_np);
		regulators_np = of_find_node_by_name(regulators_np, "regulators");
	}

	rgt_info = kzalloc(sizeof(struct exynos_rgt_info) * num_regulators, GFP_KERNEL);
	if (!rgt_info) {
		pr_err("%s %s: could not allocate mem for rgt_info\n", EXYNOS_RGT_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_rgt_info;
	}

	exynos_rgt_root = debugfs_create_dir("exynos-rgt", NULL);
	if (!exynos_rgt_root) {
		pr_err("%s %s: could not create debugfs root dir\n",
				EXYNOS_RGT_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_dbgfs_root;
	}

#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	ret = prepare_dss_regulator(pdev);
	if (ret)
		goto err_dbgfs_root;
#endif
	regulators_np = of_find_node_by_name(NULL, "regulators");
	while (regulators_np) {
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
		rgt_ss_idx = 0;
#endif
		for_each_child_of_node(regulators_np, reg_np) {
			rgt_name = of_get_property(reg_np, "regulator-name", NULL);
			if (!rgt_name)
				continue;

			rgt_info[rgt_idx].rgt = regulator_get(&pdev->dev, rgt_name);
			if (IS_ERR(rgt_info[rgt_idx].rgt)) {
				pr_err("%s %s: failed to getting regulator %s\n", EXYNOS_RGT_PREFIX, __func__, rgt_name);
				continue;
			}

			rgt_info[rgt_idx].get_fops = exynos_rgt_get_fops;
			rgt_info[rgt_idx].ena_fops = exynos_rgt_ena_fops;
			rgt_info[rgt_idx].volt_fops = exynos_rgt_volt_fops;
			rgt_info[rgt_idx].reg_dir =
				debugfs_create_dir(rgt_name, exynos_rgt_root);
			rgt_info[rgt_idx].f_get =
				debugfs_create_file("get", 0400, rgt_info[rgt_idx].reg_dir,
						rgt_info[rgt_idx].rgt, &rgt_info[rgt_idx].get_fops);
			rgt_info[rgt_idx].f_ena =
				debugfs_create_file("enable", 0600, rgt_info[rgt_idx].reg_dir,
						rgt_info[rgt_idx].rgt, &rgt_info[rgt_idx].ena_fops);
			rgt_info[rgt_idx].f_volt =
				debugfs_create_file("voltage", 0600, rgt_info[rgt_idx].reg_dir,
						rgt_info[rgt_idx].rgt, &rgt_info[rgt_idx].volt_fops);
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
			pmic_np = of_get_parent(regulators_np);
			ret = of_property_read_u32(pmic_np, "acpm-ipc-channel", &ipc_ch);
			if (!ret) {
				ret = of_property_read_u32(pmic_np, "acpm-speedy-channel", &spd_ch);
				if (!ret) {
					rgt_ss = get_regulator_ss(rgt_ss_idx++, spd_ch);
					if (rgt_ss != NULL) {
						rdesc = *(rgt_info[rgt_idx].rgt->rdev->desc);

						snprintf(rgt_ss->name, sizeof(rgt_ss->name), "%s.", rdesc.name);
						rgt_ss->min_uV = rdesc.min_uV;
						rgt_ss->uV_step = rdesc.uV_step;
						rgt_ss->linear_min_sel = rdesc.linear_min_sel;
						rgt_ss->vsel_reg = rdesc.vsel_reg;
					}
				}
			}
#endif
			rgt_idx++;
		}
		regulators_np = of_find_node_by_name(regulators_np, "regulators");
	}
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	set_reg_map();
#endif
	platform_set_drvdata(pdev, rgt_info);

	return 0;

err_dbgfs_root:
	kfree(rgt_info);
err_rgt_info:
err_find_regs:
	return ret;
}

static int exynos_rgt_remove(struct platform_device *pdev)
{
	struct exynos_rgt_info *rgt_info = platform_get_drvdata(pdev);
	int i = 0;

	for (i = 0; i < num_regulators; i++) {
		debugfs_remove_recursive(rgt_info[i].f_volt);
		debugfs_remove_recursive(rgt_info[i].f_ena);
		debugfs_remove_recursive(rgt_info[i].f_get);
		debugfs_remove_recursive(rgt_info[i].reg_dir);
		regulator_put(rgt_info[i].rgt);
	}

	debugfs_remove_recursive(exynos_rgt_root);
	kfree(rgt_info);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id exynos_rgt_match[] = {
	{
		.compatible = "samsung,exynos-rgt",
	},
	{},
};

static struct platform_driver exynos_rgt_drv = {
	.probe		= exynos_rgt_probe,
	.remove		= exynos_rgt_remove,
	.driver		= {
		.name	= "exynos_rgt",
		.owner	= THIS_MODULE,
		.of_match_table = exynos_rgt_match,
	},
};

static int __init exynos_rgt_init(void)
{
	return platform_driver_register(&exynos_rgt_drv);
}
late_initcall(exynos_rgt_init);
