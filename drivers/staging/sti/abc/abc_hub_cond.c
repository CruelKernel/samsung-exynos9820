/* abc_hub_cond.c
 *
 * Abnormal Behavior Catcher Hub Driver Sub Module(Conncet Detect)
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define DEBUG_FOR_SECDETECT
#include <linux/sti/abc_hub.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/sec_class.h>
#include <asm/io.h>

#define SEC_CONN_PRINT(format, ...) pr_info("[ABC_COND] " format, ##__VA_ARGS__)
#define NWEINT_GPM23_CON		0x15c3075C
#define WEINT_GPA3_CON		0x1585070C
#define NWEINT_GPG0_CON		0x10430710
#define WEINT_GPM18_CON		0x15c30748


static void detect_con_weint_dump(void)
{
#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
    void __iomem *ub_int_reg;
#endif
	void __iomem *sub_int_reg;
	void __iomem *cam_int_reg;

#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	u32 ub_tmp_reg;
#endif
	u32 sub_tmp_reg;
	u32 cam_tmp_reg;

#if defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	sub_int_reg = ioremap(NWEINT_GPG0_CON, 0x10);
	cam_int_reg = ioremap(WEINT_GPM18_CON, 0x10);
#else
	ub_int_reg = ioremap(NWEINT_GPM23_CON, 0x10);
	sub_int_reg = ioremap(WEINT_GPA3_CON, 0x10);
	cam_int_reg = ioremap(WEINT_GPM18_CON, 0x10);
#endif

#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	ub_tmp_reg = readl(ub_int_reg);
#endif
	sub_tmp_reg = readl(sub_int_reg);
	cam_tmp_reg = readl(cam_int_reg);
	

#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	SEC_CONN_PRINT("GPM23[0] 0x15c3075C: %#x\n", ub_tmp_reg);
#endif
#if defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	SEC_CONN_PRINT("GPG0[1] :0x10430710: %#x\n", sub_tmp_reg);
	SEC_CONN_PRINT("GPM18[0] 0x15c30748: %#x\n", cam_tmp_reg);
#else	
	SEC_CONN_PRINT("GPA3[3] :0x1585070C: %#x\n", sub_tmp_reg);
	SEC_CONN_PRINT("GPM18[0] 0x15c30748: %#x\n", cam_tmp_reg);
#endif
	

#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
	iounmap(ub_int_reg);
#endif
	iounmap(sub_int_reg);
	iounmap(cam_int_reg);
}



static irqreturn_t abc_hub_cond_interrupt_handler(int irq, void *handle)
{
	int type;
	struct abc_hub_info *pinfo = handle;
	int i;

	type = irq_get_trigger_type(irq);

	SEC_CONN_PRINT("%s\n", __func__);
	detect_con_weint_dump();
	/*Send Uevent Data*/
	for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++) {
		if (irq == pinfo->pdata->cond_pdata.irq_number[i]) {
			/* if Rising or falling edge occurs, check level one more.
			 * and if it still level high send uevent.
			 */
			if (gpio_get_value(pinfo->pdata->cond_pdata.irq_gpio[i])) {
#if !defined(CONFIG_SEC_ABC_HUB_COND_DAVINCI)
				if (i == 0)
					abc_hub_send_event("MODULE=cond@ERROR=ub");
				else if (i == 1)
					abc_hub_send_event("MODULE=cond@ERROR=sub");
				else
					abc_hub_send_event("MODULE=cond@ERROR=cam");
			}
#else
				if (i == 0)
					abc_hub_send_event("MODULE=cond@ERROR=sub");
				else if (i == 1)
					abc_hub_send_event("MODULE=cond@ERROR=cam");
				else
					abc_hub_send_event("MODULE=cond@ERROR=tof");
			}
#endif
		}
	}
	return IRQ_HANDLED;
}

int abc_hub_cond_irq_enable(struct device *dev, int enable)
{
	/* detect_conn_init_irq thread create*/
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	int i;
	int retval;

	for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++) {
		retval = request_threaded_irq(pinfo->pdata->cond_pdata.irq_number[i], NULL,
				abc_hub_cond_interrupt_handler,
				pinfo->pdata->cond_pdata.irq_type[i] | IRQF_ONESHOT,
				pinfo->pdata->cond_pdata.name[i], pinfo);
			if (retval) {
				SEC_CONN_PRINT("%s: Failed to request threaded irq %d.\n",
								__func__, retval);
				return retval;
			}
#if defined(DEBUG_FOR_SECDETECT)
			SEC_CONN_PRINT("%s: Succeeded to request threaded irq %d: irq_num[%d], type[%x],name[%s].\n",
							__func__, retval, pinfo->pdata->cond_pdata.irq_number[i],
							pinfo->pdata->cond_pdata.irq_type[i], pinfo->pdata->cond_pdata.name[i]);
#endif
	msleep(1);
	}

	return 0;
}

int parse_cond_data(struct device *dev,
		    struct abc_hub_platform_data *pdata,
		    struct device_node *np)
{
	/* implement parse dt for sub module */
	/* the following code is just example. replace it to your code */

	struct device_node *cond_np;

	int i;

	cond_np = of_find_node_by_name(np, "cond");

	SEC_CONN_PRINT("%s\n", __func__);

	pdata->cond_pdata.gpio_cnt = of_gpio_named_count(cond_np, "cond,det_conn_gpios");

	for (i = 0; i < pdata->cond_pdata.gpio_cnt; i++) {
		/*Get connector name*/
		of_property_read_string_index(cond_np, "cond,name", i, &(pdata->cond_pdata.name[i]));

		/*Get connector gpio number*/
		pdata->cond_pdata.irq_gpio[i] = of_get_named_gpio(cond_np, "cond,det_conn_gpios", i);

		if (gpio_is_valid(pdata->cond_pdata.irq_gpio[i])) {
#if defined(DEBUG_FOR_SECDETECT)
			SEC_CONN_PRINT("i = [%d] gpio level [%d] = %d\n", i, pdata->cond_pdata.irq_gpio[i],
				gpio_get_value(pdata->cond_pdata.irq_gpio[i]));
			SEC_CONN_PRINT("gpio irq gpio = [%d], irq = [%d]\n", pdata->cond_pdata.irq_gpio[i],
			gpio_to_irq(pdata->cond_pdata.irq_gpio[i]));
#endif
			/*Filling the irq_number from this gpio.*/
			pdata->cond_pdata.irq_number[i] = gpio_to_irq(pdata->cond_pdata.irq_gpio[i]);

		} else {
			dev_err(dev, "%s: Failed to get irq gpio.\n", __func__);
			return -EINVAL;
		}
	}

	/*Get type of gpio irq*/
	if (of_property_read_u32_array(cond_np, "cond,det_conn_irq_type", pdata->cond_pdata.irq_type, pdata->cond_pdata.gpio_cnt)) {
		dev_err(dev, "%s, Failed to get irq_type property.\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < pdata->cond_pdata.gpio_cnt; i++)
		SEC_CONN_PRINT("pin gpio[%d] irq number[%d] CONNECTOR_NAME=%s,\n", pdata->cond_pdata.irq_gpio[i], pdata->cond_pdata.irq_number[i], pdata->cond_pdata.name[i]);

	return 0;
}

int abc_hub_cond_init(struct device *dev)
{
	/* implement init sequence : return 0 if success, return -1 if fail */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	
	mutex_init(&pinfo->pdata->cond_pdata.cond_lock);

	return 0;
}

int abc_hub_cond_suspend(struct device *dev)
{
	/* implement suspend sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	int i;

	if (pinfo->pdata->cond_pdata.enabled == ABC_HUB_ENABLED) {
		for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++)
			disable_irq(pinfo->pdata->cond_pdata.irq_number[i]);
	}

	return 0;
}

int abc_hub_cond_resume(struct device *dev)
{
	/* implement resume sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	int i;

	if (pinfo->pdata->cond_pdata.enabled == ABC_HUB_ENABLED) {
		for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++)
			enable_irq(pinfo->pdata->cond_pdata.irq_number[i]);
	}

	return 0;
}

void abc_hub_cond_enable(struct device *dev, int enable)
{
	/* common sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	int i;
	int ret;

	mutex_lock(&pinfo->pdata->cond_pdata.cond_lock);

	if (enable == ABC_HUB_ENABLED) {
		SEC_CONN_PRINT("driver enabled.\n");
		for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++) {
			/*get level value of the gpio pin.*/
			SEC_CONN_PRINT("gpio level [%d] = %d\n", pinfo->pdata->cond_pdata.irq_gpio[i], gpio_get_value(pinfo->pdata->cond_pdata.irq_gpio[i]));
		}
		/*Enable interrupt.*/
		ret = abc_hub_cond_irq_enable(dev, enable);
		if (ret < 0)
			SEC_CONN_PRINT("Interrupt not enabled.\n");
		pinfo->pdata->cond_pdata.enabled = enable;
	} else {
		if (pinfo->pdata->cond_pdata.enabled == ABC_HUB_ENABLED) {
			for (i = 0; i < pinfo->pdata->cond_pdata.gpio_cnt; i++) {
				if (pinfo->pdata->cond_pdata.irq_number[i]) {
					disable_irq(pinfo->pdata->cond_pdata.irq_number[i]);
					free_irq(pinfo->pdata->cond_pdata.irq_number[i], pinfo);
				}
			}
		}
		pinfo->pdata->cond_pdata.enabled = enable;
	}
	
	mutex_unlock(&pinfo->pdata->cond_pdata.cond_lock);


}

MODULE_DESCRIPTION("Samsung ABC Hub Sub Module1(cond) Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
