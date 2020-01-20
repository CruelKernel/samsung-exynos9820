/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - CHIP ID support
 * Author: Pankaj Dubey <pankaj.dubey@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/soc/samsung/exynos-soc.h>

struct exynos_chipid_info exynos_soc_info;
EXPORT_SYMBOL(exynos_soc_info);

static const char *soc_ap_id;

static const char * __init product_id_to_name(unsigned int product_id)
{
	const char *soc_name;
	unsigned int soc_id = product_id;

	switch (soc_id) {
	case EXYNOS3250_SOC_ID:
		soc_name = "EXYNOS3250";
		break;
	case EXYNOS4210_SOC_ID:
		soc_name = "EXYNOS4210";
		break;
	case EXYNOS4212_SOC_ID:
		soc_name = "EXYNOS4212";
		break;
	case EXYNOS4412_SOC_ID:
		soc_name = "EXYNOS4412";
		break;
	case EXYNOS4415_SOC_ID:
		soc_name = "EXYNOS4415";
		break;
	case EXYNOS5250_SOC_ID:
		soc_name = "EXYNOS5250";
		break;
	case EXYNOS5260_SOC_ID:
		soc_name = "EXYNOS5260";
		break;
	case EXYNOS5420_SOC_ID:
		soc_name = "EXYNOS5420";
		break;
	case EXYNOS5440_SOC_ID:
		soc_name = "EXYNOS5440";
		break;
	case EXYNOS5800_SOC_ID:
		soc_name = "EXYNOS5800";
		break;
	case EXYNOS7872_SOC_ID:
		soc_name = "EXYNOS7872";
		break;
	case EXYNOS8890_SOC_ID:
		soc_name = "EXYNOS8890";
		break;
	case EXYNOS8895_SOC_ID:
		soc_name = "EXYNOS8895";
		break;
	case EXYNOS9810_SOC_ID:
		soc_name = "EXYNOS9810";
		break;
	case EXYNOS9820_SOC_ID:
		soc_name = "EXYNOS9820";
		break;
	default:
		soc_name = "UNKNOWN";
	}
	return soc_name;
}
static const struct exynos_chipid_variant drv_data_exynos8890 = {
	.product_ver	= 1,
	.unique_id_reg	= 0x14,
	.rev_reg	= 0x0,
	.main_rev_bit	= 0,
	.sub_rev_bit	= 4,
};

static const struct exynos_chipid_variant drv_data_exynos8895 = {
	.product_ver	= 1,
	.unique_id_reg	= 0x04,
	.rev_reg	= 0x10,
	.main_rev_bit	= 20,
	.sub_rev_bit	= 16,
};

static const struct exynos_chipid_variant drv_data_exynos7872 = {
	.product_ver	= 2,
	.unique_id_reg	= 0x04,
	.rev_reg	= 0x10,
	.main_rev_bit	= 20,
	.sub_rev_bit	= 16,
};

static const struct exynos_chipid_variant drv_data_exynos9810 = {
	.product_ver	= 1,
	.unique_id_reg	= 0x04,
	.rev_reg	= 0x10,
	.main_rev_bit	= 20,
	.sub_rev_bit	= 16,
};

static const struct of_device_id of_exynos_chipid_ids[] = {
	{
		.compatible	= "samsung,exynos8890-chipid",
		.data		= &drv_data_exynos8890,
	},
	{
		.compatible	= "samsung,exynos8895-chipid",
		.data		= &drv_data_exynos8895,
	},
	{
		.compatible	= "samsung,exynos7872-chipid",
		.data		= &drv_data_exynos8895,
	},
	{
		.compatible	= "samsung,exynos9810-chipid",
		.data		= &drv_data_exynos9810,
	},
	{},
};

static char lot_id[6];

static u32 chipid_reverse_value(u32 val, u32 bitcnt)
{
	u32 temp, ret = 0;
	u32 i;

	for (i = 0; i < bitcnt; i++) {
		temp = (val >> i) & 0x1;
		ret += temp << ((bitcnt - 1) - i);
	}

	return ret;
}

static void chipid_dec_to_36(u32 in, char *p)
{
	const struct exynos_chipid_variant *data = exynos_soc_info.drv_data;

	u32 mod;
	u32 i;
	u32 val;

	for (i = 4; i >= 1; i--) {
		mod = in % 36;
		in /= 36;
		p[i] = (mod < 10) ? (mod + '0') : (mod - 10 + 'A');
	}

	val = __raw_readl(exynos_soc_info.reg + data->unique_id_reg + 0x4);
	val = (val >> 10) & 0x3;

	switch (val) {
	case 0:
		p[0] = 'N';
		break;
	case 1:
		p[0] = 'S';
		break;
	case 2:
		p[0] = 'A';
		break;
	case 3:
	default:
		break;
	}

	p[5] = 0;
}

static void __init exynos_chipid_get_chipid_info(void)
{
	const struct exynos_chipid_variant *data = exynos_soc_info.drv_data;
	u64 val;
	u32 temp;

	val = __raw_readl(exynos_soc_info.reg);

	switch (data->product_ver) {
	case 2:
		exynos_soc_info.product_id = val & EXYNOS_SOC_MASK_V2;
		break;
	case 1:
	default:
		exynos_soc_info.product_id = val & EXYNOS_SOC_MASK;
		break;
	}

	val = __raw_readl(exynos_soc_info.reg + data->rev_reg);
	exynos_soc_info.main_rev = (val >> data->main_rev_bit) & EXYNOS_REV_MASK;
	exynos_soc_info.sub_rev = (val >> data->sub_rev_bit) & EXYNOS_REV_MASK;
	exynos_soc_info.revision = (exynos_soc_info.main_rev << 4) | exynos_soc_info.sub_rev;

	val = __raw_readl(exynos_soc_info.reg + data->unique_id_reg);
	val |= (u64)__raw_readl(exynos_soc_info.reg + data->unique_id_reg + 4) << 32UL;
	exynos_soc_info.unique_id  = val;
	exynos_soc_info.lot_id = val & EXYNOS_LOTID_MASK;

	temp = __raw_readl(exynos_soc_info.reg + data->unique_id_reg);
	temp = chipid_reverse_value(temp, 32);
	temp = (temp >> 11) & EXYNOS_LOTID_MASK;
	chipid_dec_to_36(temp, lot_id);
	exynos_soc_info.lot_id2 = lot_id;
}

#if defined(CONFIG_SEC_FACTORY)
#define POWER_BASE	0x15860000
#define DREX_CAL6	0x09B8

struct exynos_ddr_info {
	unsigned int lot_id;
	unsigned int rev;
};

struct exynos_ddr_info ddr_info;

static void __init exynos_chipid_get_dram_info(void)
{
	unsigned int reg;
	void __iomem *rev_addr;

	rev_addr = ioremap(POWER_BASE, SZ_32);

	reg = readl(rev_addr + DREX_CAL6);

	ddr_info.lot_id = (reg & 0x3f)*100 + ((reg >> 6) & 0x3f)* 10 + ((reg >> 12) & 0xf);
	ddr_info.rev = (reg >> 16) & 0xffff;	

	iounmap(rev_addr);
	return;
}
#else
static void __init exynos_chipid_get_dram_info(void)
{
	return;
}
#endif

/**
 *  exynos_chipid_early_init: Early chipid initialization
 *  @dev: pointer to chipid device
 */
void __init exynos_chipid_early_init(void)
{
	struct device_node *np;
	const struct of_device_id *match;

	if (exynos_soc_info.reg)
		return;

	np = of_find_matching_node_and_match(NULL, of_exynos_chipid_ids, &match);
	if (!np || !match)
		panic("%s, failed to find chipid node or match\n", __func__);

	exynos_soc_info.drv_data = (struct exynos_chipid_variant *)match->data;
	exynos_soc_info.reg = of_iomap(np, 0);
	if (!exynos_soc_info.reg)
		panic("%s: failed to map registers\n", __func__);

	exynos_chipid_get_chipid_info();
	exynos_chipid_get_dram_info();
}

static int __init exynos_chipid_probe(struct platform_device *pdev)
{
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *root;
	int ret;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENODEV;

	soc_dev_attr->family = "Samsung Exynos";

	root = of_find_node_by_path("/");
	ret = of_property_read_string(root, "model", &soc_dev_attr->machine);
	of_node_put(root);
	if (ret)
		goto free_soc;

	soc_dev_attr->revision = kasprintf(GFP_KERNEL, "%d",
					exynos_soc_info.revision);
	if (!soc_dev_attr->revision)
		goto free_soc;

	soc_dev_attr->soc_id = product_id_to_name(exynos_soc_info.product_id);
	soc_ap_id = product_id_to_name(exynos_soc_info.product_id);
	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev))
		goto free_rev;

	soc_device_to_device(soc_dev);
	dev_info(&pdev->dev, "Exynos: CPU[%s] CPU_REV[0x%x] Detected\n",
			product_id_to_name(exynos_soc_info.product_id),
			exynos_soc_info.revision);
	return 0;
free_rev:
	kfree(soc_dev_attr->revision);
free_soc:
	kfree(soc_dev_attr);
	return -EINVAL;
}

static struct platform_driver exynos_chipid_driver __refdata = {
	.driver = {
		.name = "exynos-chipid",
		.of_match_table = of_exynos_chipid_ids,
	},
	.probe = exynos_chipid_probe,
};

static int __init exynos_chipid_init(void)
{
	exynos_chipid_early_init();
	return platform_driver_register(&exynos_chipid_driver);
}
core_initcall(exynos_chipid_init);

/*
 *  sysfs implementation for exynos-snapshot
 *  you can access the sysfs of exynos-snapshot to /sys/devices/system/chip-id
 *  path.
 */
static struct bus_type chipid_subsys = {
	.name = "chip-id",
	.dev_name = "chip-id",
};

static ssize_t chipid_product_id_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%08X\n", exynos_soc_info.product_id);
}

static ssize_t chipid_unique_id_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 20, "%010LX\n", exynos_soc_info.unique_id);
}

static ssize_t chipid_lot_id_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 14, "%08X\n", exynos_soc_info.lot_id);
}

static ssize_t chipid_lot_id2_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 14, "%s\n", exynos_soc_info.lot_id2);
}

static ssize_t chipid_revision_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 14, "%08X\n", exynos_soc_info.revision);
}

static ssize_t chipid_evt_ver_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	if (exynos_soc_info.revision == 0)
		return snprintf(buf, 14, "EVT0\n");
	else
		return snprintf(buf, 14, "EVT%1X.%1X\n",
				exynos_soc_info.main_rev,
				exynos_soc_info.sub_rev);
}

static ssize_t chipid_ap_id_show(struct kobject *kobj,
                                 struct kobj_attribute *attr, char *buf)
{
	if (exynos_soc_info.revision == 0)
		return snprintf(buf, 30, "%s EVT0\n", soc_ap_id);
	else
		return snprintf(buf, 30, "%s EVT%1X.%1X\n",
				soc_ap_id,
				exynos_soc_info.main_rev,
				exynos_soc_info.sub_rev);
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t chipid_ddr_lot_id_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 14, "%d\n", ddr_info.lot_id);
}

static ssize_t chipid_ddr_rev_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 14, "%d\n", ddr_info.rev);
}
#endif

static struct kobj_attribute chipid_product_id_attr =
        __ATTR(product_id, 0644, chipid_product_id_show, NULL);

static struct kobj_attribute chipid_ap_id_attr =
        __ATTR(ap_id, 0644, chipid_ap_id_show, NULL);

static struct kobj_attribute chipid_unique_id_attr =
        __ATTR(unique_id, 0644, chipid_unique_id_show, NULL);

static struct kobj_attribute chipid_lot_id_attr =
        __ATTR(lot_id, 0644, chipid_lot_id_show, NULL);

static struct kobj_attribute chipid_lot_id2_attr =
        __ATTR(lot_id2, 0644, chipid_lot_id2_show, NULL);

static struct kobj_attribute chipid_revision_attr =
        __ATTR(revision, 0644, chipid_revision_show, NULL);

static struct kobj_attribute chipid_evt_ver_attr =
        __ATTR(evt_ver, 0644, chipid_evt_ver_show, NULL);

#ifdef CONFIG_SEC_FACTORY	
static struct kobj_attribute chipid_ddr_lot_id_attr =
        __ATTR(ddr_lot_id, 0644, chipid_ddr_lot_id_show, NULL);

static struct kobj_attribute chipid_ddr_rev_attr =
        __ATTR(ddr_rev, 0644, chipid_ddr_rev_show, NULL);
#endif

static struct attribute *chipid_sysfs_attrs[] = {
	&chipid_product_id_attr.attr,
	&chipid_ap_id_attr.attr,
	&chipid_unique_id_attr.attr,
	&chipid_lot_id_attr.attr,
	&chipid_lot_id2_attr.attr,
	&chipid_revision_attr.attr,
	&chipid_evt_ver_attr.attr,
#if defined(CONFIG_SEC_FACTORY)
	&chipid_ddr_lot_id_attr.attr,
	&chipid_ddr_rev_attr.attr,
#endif	
	NULL,
};

static struct attribute_group chipid_sysfs_group = {
	.attrs = chipid_sysfs_attrs,
};

static const struct attribute_group *chipid_sysfs_groups[] = {
	&chipid_sysfs_group,
	NULL,
};

static ssize_t svc_ap_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 20, "%010llX\n",
			(exynos_soc_info.unique_id));
}

static struct kobj_attribute svc_ap_attr =
		__ATTR(SVC_AP, 0644, svc_ap_show, NULL);

extern struct kset *devices_kset;

void sysfs_create_svc_ap(void)
{
	struct kernfs_node *svc_sd;
	struct kobject *data;
	struct kobject *ap;

	/* To find svc kobject */
	svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(svc_sd)) {
		/* try to create svc kobject */
		data = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(data))
			pr_info("Existing path sys/devices/svc : 0x%pK\n", data);
		else
			pr_info("Created sys/devices/svc svc : 0x%pK\n", data);
	} else {
		data = (struct kobject *)svc_sd->priv;
		pr_info("Found svc_sd : 0x%pK svc : 0x%pK\n", svc_sd, data);
	}

	ap = kobject_create_and_add("AP", data);
	if (IS_ERR_OR_NULL(ap))
		pr_info("Failed to create sys/devices/svc/AP : 0x%pK\n", ap);
	else
		pr_info("Success to create sys/devices/svc/AP : 0x%pK\n", ap);

	if (sysfs_create_file(ap, &svc_ap_attr.attr) < 0) {
		pr_err("failed to create sys/devices/svc/AP/SVC_AP, %s\n",
		svc_ap_attr.attr.name);
	}
}

static int __init chipid_sysfs_init(void)
{
	int ret = 0;

	ret = subsys_system_register(&chipid_subsys, chipid_sysfs_groups);
	if (ret)
		pr_err("fail to register exynos-snapshop subsys\n");

	sysfs_create_svc_ap();

	return ret;
}
late_initcall(chipid_sysfs_init);

