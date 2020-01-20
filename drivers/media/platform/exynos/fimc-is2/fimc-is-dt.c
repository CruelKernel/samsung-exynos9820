/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#include <exynos-fimc-is-module.h>
#include <exynos-fimc-is-sensor.h>
#include <exynos-fimc-is.h>
#include "fimc-is-config.h"
#include "fimc-is-dt.h"
#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"

#ifdef CONFIG_OF
static int get_pin_lookup_state(struct pinctrl *pinctrl,
	struct exynos_sensor_pin (*pin_ctrls)[GPIO_SCENARIO_MAX][GPIO_CTRL_MAX])
{
	int ret = 0;
	u32 i, j, k;
	char pin_name[30];
	struct pinctrl_state *s;

	for (i = 0; i < SENSOR_SCENARIO_MAX; ++i) {
		for (j = 0; j < GPIO_SCENARIO_MAX; ++j) {
			for (k = 0; k < GPIO_CTRL_MAX; ++k) {
				if (pin_ctrls[i][j][k].act == PIN_FUNCTION) {
					snprintf(pin_name, sizeof(pin_name), "%s%d",
						pin_ctrls[i][j][k].name,
						pin_ctrls[i][j][k].value);
					s = pinctrl_lookup_state(pinctrl, pin_name);
					if (IS_ERR_OR_NULL(s)) {
						err("pinctrl_lookup_state(%s) is failed", pin_name);
						ret = -EINVAL;
						goto p_err;
					}

					pin_ctrls[i][j][k].pin = (ulong)s;
				}
			}
		}
	}

p_err:
	return ret;
}

static int parse_gate_info(struct exynos_platform_fimc_is *pdata, struct device_node *np)
{
	int ret = 0;
	struct device_node *group_np = NULL;
	struct device_node *gate_info_np;
	struct property *prop;
	struct property *prop2;
	const __be32 *p;
	const char *s;
	u32 i = 0, u = 0;
	struct exynos_fimc_is_clk_gate_info *gate_info;

	/* get subip of fimc-is info */
	gate_info = kzalloc(sizeof(struct exynos_fimc_is_clk_gate_info), GFP_KERNEL);
	if (!gate_info) {
		printk(KERN_ERR "%s: no memory for fimc_is gate_info\n", __func__);
		return -EINVAL;
	}

	s = NULL;
	/* get gate register info */
	prop2 = of_find_property(np, "clk_gate_strs", NULL);
	of_property_for_each_u32(np, "clk_gate_enums", prop, p, u) {
		printk(KERN_INFO "int value: %d\n", u);
		s = of_prop_next_string(prop2, s);
		if (s != NULL) {
			printk(KERN_INFO "String value: %d-%s\n", u, s);
			gate_info->gate_str[u] = s;
		}
	}

	/* gate info */
	gate_info_np = of_find_node_by_name(np, "clk_gate_ctrl");
	if (!gate_info_np) {
		ret = -ENOENT;
		goto p_err;
	}
	i = 0;
	while ((group_np = of_get_next_child(gate_info_np, group_np))) {
		struct exynos_fimc_is_clk_gate_group *group =
				&gate_info->groups[i];
		of_property_for_each_u32(group_np, "mask_clk_on_org", prop, p, u) {
			printk(KERN_INFO "(%d) int1 value: %d\n", i, u);
			group->mask_clk_on_org |= (1 << u);
		}
		of_property_for_each_u32(group_np, "mask_clk_off_self_org", prop, p, u) {
			printk(KERN_INFO "(%d) int2 value: %d\n", i, u);
			group->mask_clk_off_self_org |= (1 << u);
		}
		of_property_for_each_u32(group_np, "mask_clk_off_depend", prop, p, u) {
			printk(KERN_INFO "(%d) int3 value: %d\n", i, u);
			group->mask_clk_off_depend |= (1 << u);
		}
		of_property_for_each_u32(group_np, "mask_cond_for_depend", prop, p, u) {
			printk(KERN_INFO "(%d) int4 value: %d\n", i, u);
			group->mask_cond_for_depend |= (1 << u);
		}
		i++;
		printk(KERN_INFO "(%d) [0x%x , 0x%x, 0x%x, 0x%x\n", i,
			group->mask_clk_on_org,
			group->mask_clk_off_self_org,
			group->mask_clk_off_depend,
			group->mask_cond_for_depend
		);
	}

	pdata->gate_info = gate_info;
	pdata->gate_info->clk_on_off = exynos_fimc_is_clk_gate;

	return 0;
p_err:
	kfree(gate_info);
	return ret;
}

#if defined(CONFIG_PM_DEVFREQ)
DECLARE_EXTERN_DVFS_DT(FIMC_IS_SN_END);
static int parse_dvfs_data(struct exynos_platform_fimc_is *pdata, struct device_node *np, int index)
{
	int i;
	u32 temp;
	char *pprop;
	char buf[64];

	for (i = 0; i < FIMC_IS_SN_END; i++) {
#if defined(QOS_INTCAM)
		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "int_cam");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_INT_CAM]);
#endif
		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "int");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_INT]);

		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "cam");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_CAM]);

		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "mif");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_MIF]);

		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "i2c");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_I2C]);

		sprintf(buf, "%s%s", fimc_is_dvfs_dt_arr[i].parse_scenario_nm, "hpg");
		DT_READ_U32(np, buf, pdata->dvfs_data[index][fimc_is_dvfs_dt_arr[i].scenario_id][FIMC_IS_DVFS_HPG]);
	}

#ifdef DBG_DUMP_DVFS_DT
	for (i = 0; i < FIMC_IS_SN_END; i++) {
		probe_info("---- %s ----\n", fimc_is_dvfs_dt_arr[i].parse_scenario_nm);
#if defined(QOS_INTCAM)
		probe_info("[%d][%d][INT_CAM] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_INT_CAM]);
#endif
		probe_info("[%d][%d][INT] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_INT]);
		probe_info("[%d][%d][CAM] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_CAM]);
		probe_info("[%d][%d][MIF] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_MIF]);
		probe_info("[%d][%d][I2C] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_I2C]);
		probe_info("[%d][%d][HPG] = %d\n", index, i, pdata->dvfs_data[index][i][FIMC_IS_DVFS_HPG]);
	}
#endif
	return 0;
}
#else
static int parse_dvfs_data(struct exynos_platform_fimc_is *pdata, struct device_node *np, int index)
{
	return 0;
}
#endif

static int parse_dvfs_table(struct fimc_is_dvfs_ctrl *dvfs,
	struct exynos_platform_fimc_is *pdata, struct device_node *np)
{
	int ret = 0;
	u32 table_cnt;
	struct device_node *table_np;
	const char *dvfs_table_desc;

	table_np = NULL;

	table_cnt = 0;
	while ((table_np = of_get_next_child(np, table_np)) &&
		(table_cnt < FIMC_IS_DVFS_TABLE_IDX_MAX)) {
		ret = of_property_read_string(table_np, "desc", &dvfs_table_desc);
		if (ret)
			dvfs_table_desc = "NOT defined";

		probe_info("dvfs table[%d] is %s", table_cnt, dvfs_table_desc);
		parse_dvfs_data(pdata, table_np, table_cnt);
		table_cnt++;
	}

	dvfs->dvfs_table_max = table_cnt;

	return ret;
}

int fimc_is_parse_dt(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_dvfs_ctrl *dvfs;
	struct exynos_platform_fimc_is *pdata;
	struct device *dev;
	struct device_node *dvfs_np = NULL;
	struct device_node *vender_np = NULL;
	struct device_node *np;
	u32 mem_info[2];

	FIMC_BUG(!pdev);

	dev = &pdev->dev;
	np = dev->of_node;

	core = dev_get_drvdata(&pdev->dev);
	if (!core) {
		probe_err("core is NULL");
		return -ENOMEM;
	}

	pdata = kzalloc(sizeof(struct exynos_platform_fimc_is), GFP_KERNEL);
	if (!pdata) {
		probe_err("no memory for platform data");
		return -ENOMEM;
	}

	dvfs = &core->resourcemgr.dvfs_ctrl;
	pdata->clk_get = exynos_fimc_is_clk_get;
	pdata->clk_cfg = exynos_fimc_is_clk_cfg;
	pdata->clk_on = exynos_fimc_is_clk_on;
	pdata->clk_off = exynos_fimc_is_clk_off;
	pdata->print_clk = exynos_fimc_is_print_clk;

	if (parse_gate_info(pdata, np) < 0)
		probe_info("can't parse clock gate info node");

	of_property_read_u32(np, "chain_config", &core->chain_config);
	probe_info("FIMC-IS chain configuration: %d\n", core->chain_config);

	ret = of_property_read_u32_array(np, "secure_mem_info", mem_info, 2);
	if (ret) {
		core->secure_mem_info[0] = 0;
		core->secure_mem_info[1] = 0;
	} else {
		core->secure_mem_info[0] = mem_info[0];
		core->secure_mem_info[1] = mem_info[1];
	}
	probe_info("ret(%d) secure_mem_info(%#08lx, %#08lx)", ret,
		core->secure_mem_info[0], core->secure_mem_info[1]);

	ret = of_property_read_u32_array(np, "non_secure_mem_info", mem_info, 2);
	if (ret) {
		core->non_secure_mem_info[0] = 0;
		core->non_secure_mem_info[1] = 0;
	} else {
		core->non_secure_mem_info[0] = mem_info[0];
		core->non_secure_mem_info[1] = mem_info[1];
	}
	probe_info("ret(%d) non_secure_mem_info(%#08lx, %#08lx)", ret,
		core->non_secure_mem_info[0], core->non_secure_mem_info[1]);

	vender_np = of_find_node_by_name(np, "vender");
	if (vender_np) {
		ret = fimc_is_vender_dt(vender_np);
		if (ret)
			probe_err("fimc_is_vender_dt is fail(%d)", ret);
	}

	pdata->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pdata->pinctrl)) {
		probe_err("devm_pinctrl_get is fail");
		goto p_err;
	}

	dvfs_np = of_find_node_by_name(np, "fimc_is_dvfs");
	if (dvfs_np) {
		ret = parse_dvfs_table(dvfs, pdata, dvfs_np);
		if (ret)
			probe_err("parse_dvfs_table is fail(%d)", ret);
	}

	dev->platform_data = pdata;

	return 0;

p_err:
	kfree(pdata);
	return ret;
}

int fimc_is_sensor_parse_dt(struct platform_device *pdev)
{
	int ret;
	struct exynos_platform_fimc_is_sensor *pdata;
	struct device_node *dnode;
	struct device *dev;
	int elems;
	int i;

	FIMC_BUG(!pdev);
	FIMC_BUG(!pdev->dev.of_node);

	dev = &pdev->dev;
	dnode = dev->of_node;

	pdata = kzalloc(sizeof(struct exynos_platform_fimc_is_sensor), GFP_KERNEL);
	if (!pdata) {
		err("%s: no memory for platform data", __func__);
		return -ENOMEM;
	}

	pdata->iclk_cfg = exynos_fimc_is_sensor_iclk_cfg;
	pdata->iclk_on = exynos_fimc_is_sensor_iclk_on;
	pdata->iclk_off = exynos_fimc_is_sensor_iclk_off;
	pdata->mclk_on = exynos_fimc_is_sensor_mclk_on;
	pdata->mclk_off = exynos_fimc_is_sensor_mclk_off;
#ifdef CONFIG_SOC_EXYNOS9820
	pdata->mclk_force_off = is_sensor_mclk_force_off;
#endif

	ret = of_property_read_u32(dnode, "id", &pdata->id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto err_read_id;
	}

	ret = of_property_read_u32(dnode, "scenario", &pdata->scenario);
	if (ret) {
		err("scenario read is fail(%d)", ret);
		goto err_read_scenario;
	}

	ret = of_property_read_u32(dnode, "csi_ch", &pdata->csi_ch);
	if (ret) {
		err("csi_ch read is fail(%d)", ret);
		goto err_read_csi_ch;
	}

	elems = of_property_count_u32_elems(dnode, "dma_ch");
	if (elems >= CSI_VIRTUAL_CH_MAX) {
		if (elems % CSI_VIRTUAL_CH_MAX) {
			err("the length of DMA ch. is not a multiple of VC Max");
			ret = -EINVAL;
			goto err_read_dma_ch;
		}

		if (elems != of_property_count_u32_elems(dnode, "vc_ch")) {
			err("the length of DMA ch. does not match VC ch.");
			ret = -EINVAL;
			goto err_read_vc_ch;
		}

		pdata->dma_ch = kcalloc(elems, sizeof(*pdata->dma_ch), GFP_KERNEL);
		if (!pdata->dma_ch) {
			err("out of memory for DMA ch.");
			ret = -EINVAL;
			goto err_alloc_dma_ch;
		}

		pdata->vc_ch = kcalloc(elems, sizeof(*pdata->vc_ch), GFP_KERNEL);
		if (!pdata->vc_ch) {
			err("out of memory for VC ch.");
			ret = -EINVAL;
			goto err_alloc_vc_ch;
		}

		for (i = 0; i < elems; i++) {
			pdata->dma_ch[i] = -1;
			pdata->vc_ch[i] = -1;
		}

		if (!of_property_read_u32_array(dnode, "dma_ch", pdata->dma_ch,
					elems)) {
			if (!of_property_read_u32_array(dnode, "vc_ch",
						pdata->vc_ch,
						elems)) {
				pdata->dma_abstract = true;
				pdata->num_of_ch_mode = elems / CSI_VIRTUAL_CH_MAX;
			} else {
				warn("failed to read vc_ch\n");
			}
		} else {
			warn("failed to read dma_ch\n");
		}

		if (!pdata->dma_abstract) {
			kfree(pdata->vc_ch);
			kfree(pdata->dma_ch);
		}
	}

	ret = of_property_read_u32(dnode, "flite_ch", &pdata->flite_ch);
	if (ret) {
		err("flite_ch read is fail(%d)", ret);
		goto err_read_flite_ch;
	}

	ret = of_property_read_u32(dnode, "is_bns", &pdata->is_bns);
	if (ret) {
		err("is_bns read is fail(%d)", ret);
		goto err_read_is_bns;
	}

	if (of_property_read_bool(dnode, "use_ssvc0_internal"))
		set_bit(SUBDEV_SSVC0_INTERNAL_USE, &pdata->internal_state);

	if (of_property_read_bool(dnode, "use_ssvc1_internal"))
		set_bit(SUBDEV_SSVC1_INTERNAL_USE, &pdata->internal_state);

	if (of_property_read_bool(dnode, "use_ssvc2_internal"))
		set_bit(SUBDEV_SSVC2_INTERNAL_USE, &pdata->internal_state);

	if (of_property_read_bool(dnode, "use_ssvc3_internal"))
		set_bit(SUBDEV_SSVC3_INTERNAL_USE, &pdata->internal_state);

	pdev->id = pdata->id;
	dev->platform_data = pdata;

	return 0;

err_read_is_bns:
err_read_flite_ch:
	kfree(pdata->vc_ch);

err_alloc_vc_ch:
	kfree(pdata->dma_ch);

err_alloc_dma_ch:
err_read_vc_ch:
err_read_dma_ch:
err_read_csi_ch:
err_read_scenario:
err_read_id:
	kfree(pdata);

	return ret;
}

int fimc_is_preprocessor_parse_dt(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_platform_fimc_is_preproc *pdata;
	struct device_node *dnode;
	struct device *dev;

	FIMC_BUG(!pdev);
	FIMC_BUG(!pdev->dev.of_node);

	dev = &pdev->dev;
	dnode = dev->of_node;

	pdata = kzalloc(sizeof(struct exynos_platform_fimc_is_preproc), GFP_KERNEL);
	if (!pdata) {
		probe_err("%s: no memory for platform data\n", __func__);
		return -ENOMEM;
	}

	ret = of_property_read_u32(dnode, "scenario", &pdata->scenario);
	if (ret) {
		probe_err("scenario read is fail(%d)", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "mclk_ch", &pdata->mclk_ch);
	if (ret) {
		probe_err("mclk_ch read is fail(%d)", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "id", &pdata->id);
	if (ret) {
		probe_err("csi_ch read is fail(%d)", ret);
		goto p_err;
	}

	pdata->iclk_cfg = exynos_fimc_is_preproc_iclk_cfg;
	pdata->iclk_on = exynos_fimc_is_preproc_iclk_on;
	pdata->iclk_off = exynos_fimc_is_preproc_iclk_off;
	pdata->mclk_on = exynos_fimc_is_preproc_mclk_on;
	pdata->mclk_off = exynos_fimc_is_preproc_mclk_off;

	dev->platform_data = pdata;

	return ret;

p_err:
	kfree(pdata);
	return ret;
}

static int parse_af_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->af_product_name);
	DT_READ_U32(dnode, "i2c_addr", pdata->af_i2c_addr);
	DT_READ_U32(dnode, "i2c_ch", pdata->af_i2c_ch);

	return 0;
}

static int parse_flash_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->flash_product_name);
	DT_READ_U32(dnode, "flash_first_gpio", pdata->flash_first_gpio);
	DT_READ_U32(dnode, "flash_second_gpio", pdata->flash_second_gpio);

	return 0;
}

static int parse_preprocessor_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->preprocessor_product_name);
	DT_READ_U32(dnode, "spi_channel", pdata->preprocessor_spi_channel);
	DT_READ_U32(dnode, "i2c_addr", pdata->preprocessor_i2c_addr);
	DT_READ_U32(dnode, "i2c_ch", pdata->preprocessor_i2c_ch);
	DT_READ_U32_DEFAULT(dnode, "dma_ch", pdata->preprocessor_dma_channel, DMA_CH_NOT_DEFINED);

	return 0;
}

static int parse_ois_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->ois_product_name);
	DT_READ_U32(dnode, "i2c_addr", pdata->ois_i2c_addr);
	DT_READ_U32(dnode, "i2c_ch", pdata->ois_i2c_ch);

	return 0;
}

static int parse_mcu_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->mcu_product_name);
	DT_READ_U32(dnode, "i2c_addr", pdata->mcu_i2c_addr);
	DT_READ_U32(dnode, "i2c_ch", pdata->mcu_i2c_ch);

	return 0;
}

static int parse_aperture_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(dnode, "product_name", pdata->aperture_product_name);
	DT_READ_U32(dnode, "i2c_addr", pdata->aperture_i2c_addr);
	DT_READ_U32(dnode, "i2c_ch", pdata->aperture_i2c_ch);

	return 0;
}

static int parse_power_seq_data(struct exynos_platform_fimc_is_module *pdata, struct device_node *dnode)
{
	u32 temp;
	char *pprop;
	char *name;
	struct device_node *sn_np, *seq_np;

	for_each_child_of_node(dnode, sn_np) {
		u32 sensor_scenario, gpio_scenario;

		DT_READ_U32(sn_np, "sensor_scenario", sensor_scenario);
		DT_READ_U32(sn_np, "gpio_scenario",gpio_scenario);

		pr_info("power_seq[%s] : sensor_scenario=%d, gpio_scenario=%d\n",
			sn_np->name, sensor_scenario, gpio_scenario);

		SET_PIN_INIT(pdata, sensor_scenario, gpio_scenario);

		for_each_child_of_node(sn_np, seq_np) {
			struct exynos_sensor_pin sensor_pin;
			char* pin_name;

			memset(&sensor_pin, 0, sizeof(struct exynos_sensor_pin));

			DT_READ_STR(seq_np, "pin", pin_name);
			if(!strcmp(pin_name, "gpio_none"))
				sensor_pin.pin = 0;
			else
				sensor_pin.pin = of_get_named_gpio(dnode->parent, pin_name, 0);

			DT_READ_STR(seq_np, "pname", sensor_pin.name);
			if(sensor_pin.name[0] == '\0')
				sensor_pin.name = NULL;

			DT_READ_U32(seq_np, "act", sensor_pin.act);
			DT_READ_U32(seq_np, "value", sensor_pin.value);
			DT_READ_U32(seq_np, "delay", sensor_pin.delay);
			DT_READ_U32(seq_np, "voltage", sensor_pin.voltage);

			pr_debug("power_seq node_name=%s\n", seq_np->full_name);
			pr_info("power_seq SET_PIN: pin_name=%s, name=%s, act=%d, value=%d, delay=%d, voltage=%d\n",
				pin_name, sensor_pin.name, sensor_pin.act, sensor_pin.value, sensor_pin.delay, sensor_pin.voltage);

			SET_PIN_VOLTAGE(pdata, sensor_scenario, gpio_scenario, sensor_pin.pin, sensor_pin.name,
				sensor_pin.act, sensor_pin.value, sensor_pin.delay, sensor_pin.voltage);
		}
	}

	return 0;
}

int fimc_is_module_parse_dt(struct device *dev,
	fimc_is_moudle_callback module_callback)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *pdata;
	struct device_node *dnode;
	struct device_node *af_np;
	struct device_node *flash_np;
	struct device_node *preprocessor_np;
	struct device_node *ois_np;
	struct device_node *mcu_np;
	struct device_node *aperture_np;
	struct device_node *power_np;

	FIMC_BUG(!dev);
	FIMC_BUG(!dev->of_node);
	FIMC_BUG(!module_callback);

	dnode = dev->of_node;
	pdata = kzalloc(sizeof(struct exynos_platform_fimc_is_module), GFP_KERNEL);
	if (!pdata) {
		probe_err("%s: no memory for platform data", __func__);
		return -ENOMEM;
	}

	pdata->gpio_cfg = exynos_fimc_is_module_pins_cfg;
	pdata->gpio_dbg = exynos_fimc_is_module_pins_dbg;

	ret = of_property_read_u32(dnode, "id", &pdata->id);
	if (ret) {
		probe_err("id read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "mclk_ch", &pdata->mclk_ch);
	if (ret) {
		probe_err("mclk_ch read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "sensor_i2c_ch", &pdata->sensor_i2c_ch);
	if (ret) {
		probe_info("i2c_ch dt parsing skipped\n");
	}

	ret = of_property_read_u32(dnode, "sensor_i2c_addr", &pdata->sensor_i2c_addr);
	if (ret) {
		probe_info("i2c_addr dt parsing skipped\n");
	}

	ret = of_property_read_u32(dnode, "position", &pdata->position);
	if (ret) {
		probe_err("position read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "rom_id", &pdata->rom_id);
	if (ret) {
		probe_info("rom_id dt parsing skipped\n");
		pdata->rom_id = ROM_ID_NOTHING;
	}

	ret = of_property_read_u32(dnode, "rom_type", &pdata->rom_type);
	if (ret) {
		probe_info("rom_type dt parsing skipped\n");
		pdata->rom_type = ROM_TYPE_NONE;
	}

	ret = of_property_read_u32(dnode, "rom_cal_index", &pdata->rom_cal_index);
	if (ret) {
		probe_info("rom_cal_index dt parsing skipped\n");
		pdata->rom_cal_index = ROM_CAL_NOTHING;
	}

	ret = of_property_read_u32(dnode, "rom_dualcal_id", &pdata->rom_dualcal_id);
	if (ret) {
		probe_info("rom_dualcal_id dt parsing skipped\n");
		pdata->rom_dualcal_id = ROM_ID_NOTHING;
	}


	ret = of_property_read_u32(dnode, "rom_dualcal_index", &pdata->rom_dualcal_index);
	if (ret) {
		probe_info("rom_dualcal_index dt parsing skipped\n");
		pdata->rom_dualcal_index = ROM_DUALCAL_NOTHING;
	}

	af_np = of_find_node_by_name(dnode, "af");
	if (!af_np) {
		pdata->af_product_name = ACTUATOR_NAME_NOTHING;
	} else {
		parse_af_data(pdata, af_np);
	}

	flash_np = of_find_node_by_name(dnode, "flash");
	if (!flash_np) {
		pdata->flash_product_name = FLADRV_NAME_NOTHING;
	} else {
		parse_flash_data(pdata, flash_np);
	}

	preprocessor_np = of_find_node_by_name(dnode, "preprocessor");
	if (!preprocessor_np) {
		pdata->preprocessor_product_name = PREPROCESSOR_NAME_NOTHING;
	} else {
		parse_preprocessor_data(pdata, preprocessor_np);
	}

	ois_np = of_find_node_by_name(dnode, "ois");
	if (!ois_np) {
		pdata->ois_product_name = OIS_NAME_NOTHING;
	} else {
		parse_ois_data(pdata, ois_np);
	}

	mcu_np = of_find_node_by_name(dnode, "mcu");
	if (!mcu_np) {
		pdata->mcu_product_name = MCU_NAME_NOTHING;
	} else {
		parse_mcu_data(pdata, mcu_np);
	}

	aperture_np = of_find_node_by_name(dnode, "aperture");
	if (!aperture_np) {
		pdata->aperture_product_name = APERTURE_NAME_NOTHING;
	} else {
		parse_aperture_data(pdata, aperture_np);
	}

	pdata->power_seq_dt = of_property_read_bool(dnode, "use_power_seq");
	if(pdata->power_seq_dt == true) {
		power_np = of_find_node_by_name(dnode, "power_seq");
		if (!power_np) {
			probe_err("power sequence is not declared to DT");
			goto p_err;
		} else {
			parse_power_seq_data(pdata, power_np);
		}
	} else {
		ret = module_callback(dev, pdata);
		if (ret) {
			probe_err("sensor dt callback is fail(%d)", ret);
			goto p_err;
		}
	}

	pdata->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pdata->pinctrl)) {
		probe_err("devm_pinctrl_get is fail");
		goto p_err;
	}

	ret = get_pin_lookup_state(pdata->pinctrl, pdata->pin_ctrls);
	if (ret) {
		probe_err("get_pin_lookup_state is fail(%d)", ret);
		goto p_err;
	}

	dev->platform_data = pdata;

	return ret;

p_err:
	kfree(pdata);
	return ret;
}

#ifdef CONFIG_SPI
int fimc_is_spi_parse_dt(struct fimc_is_spi *spi)
{
	int ret = 0;
	struct device_node *np;
	struct device *dev;
	struct pinctrl_state *s;

	FIMC_BUG(!spi);

	dev = &spi->device->dev;

	np = of_find_compatible_node(NULL,NULL, spi->node);
	if(np == NULL) {
		probe_err("compatible: fail to read, spi_parse_dt");
		ret = -ENODEV;
		goto p_err;
	}

	spi->use_spi_pinctrl = of_property_read_bool(np, "use_spi_pinctrl");
	if (!spi->use_spi_pinctrl) {
		probe_info("%s: spi dt parsing skipped\n", __func__);
		goto p_err;
	}

	spi->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(spi->pinctrl)) {
		probe_err("devm_pinctrl_get is fail");
		goto p_err;
	}

	s = pinctrl_lookup_state(spi->pinctrl, "ssn_out");
	if (IS_ERR_OR_NULL(s)) {
		probe_info("pinctrl_lookup_state(%s) is not found", "ssn_out");
		spi->pin_ssn_out = NULL;
	} else {
		spi->pin_ssn_out = s;
	}

	s = pinctrl_lookup_state(spi->pinctrl, "ssn_fn");
	if (IS_ERR_OR_NULL(s)) {
		probe_info("pinctrl_lookup_state(%s) is not found", "ssn_fn");
		spi->pin_ssn_fn = NULL;
	} else {
		spi->pin_ssn_fn = s;
	}

	s = pinctrl_lookup_state(spi->pinctrl, "ssn_inpd");
	if (IS_ERR_OR_NULL(s)) {
		probe_info("pinctrl_lookup_state(%s) is not found", "ssn_inpd");
		spi->pin_ssn_inpd = NULL;
	} else {
		spi->pin_ssn_inpd = s;
	}

	s = pinctrl_lookup_state(spi->pinctrl, "ssn_inpu");
	if (IS_ERR_OR_NULL(s)) {
		probe_info("pinctrl_lookup_state(%s) is not found", "ssn_inpu");
		spi->pin_ssn_inpu = NULL;
	} else {
		spi->pin_ssn_inpu = s;
	}

	spi->parent_pinctrl = devm_pinctrl_get(spi->device->dev.parent->parent);

	s = pinctrl_lookup_state(spi->parent_pinctrl, "spi_out");
	if (IS_ERR_OR_NULL(s)) {
		err("pinctrl_lookup_state(%s) is failed", "spi_out");
		ret = -EINVAL;
		goto p_err;
	}

	spi->parent_pin_out = s;

	s = pinctrl_lookup_state(spi->parent_pinctrl, "spi_fn");
	if (IS_ERR_OR_NULL(s)) {
		err("pinctrl_lookup_state(%s) is failed", "spi_fn");
		ret = -EINVAL;
		goto p_err;
	}

	spi->parent_pin_fn = s;

p_err:
	return ret;
}
#endif
#else
struct exynos_platform_fimc_is *fimc_is_parse_dt(struct device *dev)
{
	return ERR_PTR(-EINVAL);
}
#endif
