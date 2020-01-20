/*
 * max77865-irq.c - Interrupt controller support for MAX77865
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Insun Choi <insun77.choi@samsung.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max77865-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/max77865.h>
#include <linux/mfd/max77865-private.h>

static const u8 max77865_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[SYS_INT] = MAX77865_PMIC_REG_SYSTEM_INT_MASK,
	[CHG_INT] = MAX77865_CHG_REG_INT_MASK,
	[FUEL_INT] = MAX77865_REG_INVALID,
};

static struct i2c_client *get_i2c(struct max77865_dev *max77865,
				enum max77865_irq_source src)
{
	switch (src) {
	case SYS_INT:
		return max77865->i2c;
	case FUEL_INT:
		return max77865->fuelgauge;
	case CHG_INT:
		return max77865->charger;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct max77865_irq_data {
	int mask;
	enum max77865_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct max77865_irq_data max77865_irqs[] = {
	DECLARE_IRQ(MAX77865_SYSTEM_IRQ_BSTEN_INT,	SYS_INT, 1 << 3),
	DECLARE_IRQ(MAX77865_SYSTEM_IRQ_SYSUVLO_INT,	SYS_INT, 1 << 4),
	DECLARE_IRQ(MAX77865_SYSTEM_IRQ_SYSOVLO_INT,	SYS_INT, 1 << 5),
	DECLARE_IRQ(MAX77865_SYSTEM_IRQ_TSHDN_INT,	SYS_INT, 1 << 6),
	DECLARE_IRQ(MAX77865_SYSTEM_IRQ_TM_INT,		SYS_INT, 1 << 7),

	DECLARE_IRQ(MAX77865_CHG_IRQ_BYP_I,	CHG_INT, 1 << 0),
	DECLARE_IRQ(MAX77865_CHG_IRQ_BATP_I,	CHG_INT, 1 << 2),
	DECLARE_IRQ(MAX77865_CHG_IRQ_BAT_I,	CHG_INT, 1 << 3),
	DECLARE_IRQ(MAX77865_CHG_IRQ_CHG_I,	CHG_INT, 1 << 4),
	DECLARE_IRQ(MAX77865_CHG_IRQ_WCIN_I,	CHG_INT, 1 << 5),
	DECLARE_IRQ(MAX77865_CHG_IRQ_CHGIN_I,	CHG_INT, 1 << 6),
	DECLARE_IRQ(MAX77865_CHG_IRQ_AICL_I,	CHG_INT, 1 << 7),

	DECLARE_IRQ(MAX77865_FG_IRQ_ALERT, FUEL_INT, 1 << 1),
};

static void max77865_irq_lock(struct irq_data *data)
{
	struct max77865_dev *max77865 = irq_get_chip_data(data->irq);

	mutex_lock(&max77865->irqlock);
}

static void max77865_irq_sync_unlock(struct irq_data *data)
{
	struct max77865_dev *max77865 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77865_mask_reg); i++) {
		u8 mask_reg = max77865_mask_reg[i];
		struct i2c_client *i2c = get_i2c(max77865, i);

		if (mask_reg == MAX77865_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		max77865->irq_masks_cache[i] = max77865->irq_masks_cur[i];

		max77865_write_reg(i2c, max77865_mask_reg[i],
				max77865->irq_masks_cur[i]);
	}

	mutex_unlock(&max77865->irqlock);
}

static const inline struct max77865_irq_data *
irq_to_max77865_irq(struct max77865_dev *max77865, int irq)
{
	return &max77865_irqs[irq - max77865->irq_base];
}

static void max77865_irq_mask(struct irq_data *data)
{
	struct max77865_dev *max77865 = irq_get_chip_data(data->irq);
	const struct max77865_irq_data *irq_data =
	    irq_to_max77865_irq(max77865, data->irq);

	if (irq_data->group >= MAX77865_IRQ_GROUP_NR)
		return;

	max77865->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void max77865_irq_unmask(struct irq_data *data)
{
	struct max77865_dev *max77865 = irq_get_chip_data(data->irq);
	const struct max77865_irq_data *irq_data =
	    irq_to_max77865_irq(max77865, data->irq);

	if (irq_data->group >= MAX77865_IRQ_GROUP_NR)
		return;

	max77865->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static void max77865_irq_disable(struct irq_data *data)
{
	max77865_irq_mask(data);
}

static struct irq_chip max77865_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= max77865_irq_lock,
	.irq_bus_sync_unlock	= max77865_irq_sync_unlock,
	.irq_mask		= max77865_irq_mask,
	.irq_unmask		= max77865_irq_unmask,
	.irq_disable            = max77865_irq_disable,
};

static irqreturn_t max77865_irq_thread(int irq, void *data)
{
	struct max77865_dev *max77865 = data;
	u8 irq_reg[MAX77865_IRQ_GROUP_NR] = {0};
	u8 irq_src;
	int i, ret;

	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(max77865->irq_gpio));

	ret = max77865_read_reg(max77865->i2c,
					MAX77865_PMIC_REG_INTSRC, &irq_src);
	if (ret) {
		pr_err("%s:%s Failed to read interrupt source: %d\n",
			MFD_DEV_NAME, __func__, ret);
		return IRQ_NONE;
	}

	pr_info("%s:%s: irq[%d] %d/%d/%d irq_src=0x%02x\n", MFD_DEV_NAME, __func__,
		irq, max77865->irq, max77865->irq_base, max77865->irq_gpio, irq_src);


	if (irq_src & MAX77865_IRQSRC_CHG) {
		/* CHG_INT */
		ret = max77865_read_reg(max77865->charger, MAX77865_CHG_REG_INT,
					&irq_reg[CHG_INT]);
		pr_debug("%s: charger interrupt(0x%02x)\n",
				__func__, irq_reg[CHG_INT]);
		/* mask chgin to prevent chgin infinite interrupt
		 * chgin is unmasked chgin isr
		 */
		if (irq_reg[CHG_INT] &
				max77865_irqs[MAX77865_CHG_IRQ_CHGIN_I].mask) {
			u8 reg_data;
			max77865_read_reg(max77865->charger,
				MAX77865_CHG_REG_INT_MASK, &reg_data);
			reg_data |= (1 << 6);
			max77865_write_reg(max77865->charger,
				MAX77865_CHG_REG_INT_MASK, reg_data);
		}
	}


	if (irq_src & MAX77865_IRQSRC_FG) {
		pr_debug("[%s] fuelgauge interrupt\n", __func__);
		pr_debug("[%s]IRQ_BASE(%d), NESTED_IRQ(%d)\n",
			__func__, max77865->irq_base, max77865->irq_base + MAX77865_FG_IRQ_ALERT);
		handle_nested_irq(max77865->irq_base + MAX77865_FG_IRQ_ALERT);
		return IRQ_HANDLED;
	}

	if (irq_src & MAX77865_IRQSRC_TOP) {
		/* SYS_INT */
		ret = max77865_read_reg(max77865->i2c, MAX77865_PMIC_REG_SYSTEM_INT,
				&irq_reg[SYS_INT]);
		pr_debug("%s: topsys interrupt(0x%02x)\n", __func__, irq_reg[SYS_INT]);
	}

	/* Apply masking */
	for (i = 0; i < MAX77865_IRQ_GROUP_NR; i++) {
		irq_reg[i] &= ~max77865->irq_masks_cur[i];
	}

	/* Report */
	for (i = 0; i < MAX77865_IRQ_NR; i++) {
		if (irq_reg[max77865_irqs[i].group] & max77865_irqs[i].mask)
			handle_nested_irq(max77865->irq_base + i);
	}

	return IRQ_HANDLED;
}

int max77865_irq_init(struct max77865_dev *max77865)
{
	int i;
	int ret;
	u8 i2c_data;

	if (!max77865->irq_gpio) {
		dev_warn(max77865->dev, "No interrupt specified.\n");
		max77865->irq_base = 0;
		return 0;
	}

	if (!max77865->irq_base) {
		dev_err(max77865->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&max77865->irqlock);

	max77865->irq = gpio_to_irq(max77865->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			max77865->irq, max77865->irq_gpio);

	ret = gpio_request(max77865->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(max77865->dev, "%s: failed requesting gpio %d\n",
			__func__, max77865->irq_gpio);
		return ret;
	}
	gpio_direction_input(max77865->irq_gpio);
	gpio_free(max77865->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < ARRAY_SIZE(max77865_mask_reg); i++) {
		struct i2c_client *i2c;
		/* MUIC IRQ  0:MASK 1:NOT MASK => NOT USE */
		/* Other IRQ 1:MASK 0:NOT MASK */
		max77865->irq_masks_cur[i] = 0xff;
		max77865->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(max77865, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (max77865_mask_reg[i] == MAX77865_REG_INVALID)
			continue;
		max77865_write_reg(i2c, max77865_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < MAX77865_IRQ_NR; i++) {
		int cur_irq;
		cur_irq = i + max77865->irq_base;
		irq_set_chip_data(cur_irq, max77865);
		irq_set_chip_and_handler(cur_irq, &max77865_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(max77865->irq, NULL, max77865_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "max77865-irq", max77865);
	if (ret) {
		dev_err(max77865->dev, "Failed to request IRQ %d: %d\n",
			max77865->irq, ret);
		return ret;
	}


	/* Unmask max77865 interrupt */
	ret = max77865_read_reg(max77865->i2c, MAX77865_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read muic reg\n", MFD_DEV_NAME, __func__);
		return ret;
	}

	i2c_data &= ~(MAX77865_IRQSRC_CHG);	/* Unmask charger interrupt */
	i2c_data &= ~(MAX77865_IRQSRC_FG);      /* Unmask fg interrupt */

	max77865_write_reg(max77865->i2c, MAX77865_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	pr_info("%s:%s max77865_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	return 0;
}

void max77865_irq_exit(struct max77865_dev *max77865)
{
	if (max77865->irq)
		free_irq(max77865->irq, max77865);
}
