/*
 * s2mpb02-irq.c - Interrupt controller support for S2MPB02
 *
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
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
 * This driver is based on max77804-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/samsung/s2mpb02.h>
#include <linux/mfd/samsung/s2mpb02-regulator.h>

static const u8 s2mpb02_mask_reg[] = {
	[LED_INT] = S2MPB02_REG_INT1M,
};

struct s2mpb02_irq_data {
	int mask;
	enum s2mpb02_irq_source group;
};

static const struct s2mpb02_irq_data s2mpb02_irqs[] = {
	[S2MPB02_LED_IRQ_IRLED_END] = {
			.group = LED_INT,
			.mask = 1 << 5
	},
};

static void s2mpb02_irq_lock(struct irq_data *data)
{
	struct s2mpb02_dev *s2mpb02 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mpb02->irqlock);
}

static void s2mpb02_irq_sync_unlock(struct irq_data *data)
{
	struct s2mpb02_dev *s2mpb02 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MPB02_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mpb02_mask_reg[i];
		struct i2c_client *i2c = s2mpb02->i2c;

		if (mask_reg == S2MPB02_REG_INVALID || IS_ERR_OR_NULL(i2c))
			continue;
		s2mpb02->irq_masks_cache[i] = s2mpb02->irq_masks_cur[i];

		s2mpb02_write_reg(i2c, s2mpb02_mask_reg[i],
				s2mpb02->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mpb02->irqlock);
}

static const inline struct s2mpb02_irq_data *
irq_to_s2mpb02_irq(struct s2mpb02_dev *s2mpb02, int irq)
{
	return &s2mpb02_irqs[irq - s2mpb02->irq_base];
}

static void s2mpb02_irq_mask(struct irq_data *data)
{
	struct s2mpb02_dev *s2mpb02 = irq_get_chip_data(data->irq);
	const struct s2mpb02_irq_data *irq_data =
	    irq_to_s2mpb02_irq(s2mpb02, data->irq);

	if (irq_data->group >= S2MPB02_IRQ_GROUP_NR)
		return;

	s2mpb02->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void s2mpb02_irq_unmask(struct irq_data *data)
{
	struct s2mpb02_dev *s2mpb02 = irq_get_chip_data(data->irq);
	const struct s2mpb02_irq_data *irq_data =
	    irq_to_s2mpb02_irq(s2mpb02, data->irq);

	if (irq_data->group >= S2MPB02_IRQ_GROUP_NR)
		return;

	s2mpb02->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip s2mpb02_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= s2mpb02_irq_lock,
	.irq_bus_sync_unlock	= s2mpb02_irq_sync_unlock,
	.irq_mask		= s2mpb02_irq_mask,
	.irq_unmask		= s2mpb02_irq_unmask,
};

static irqreturn_t s2mpb02_irq_thread(int irq, void *data)
{
	struct s2mpb02_dev *s2mpb02 = data;
	u8 irq_reg[S2MPB02_IRQ_GROUP_NR] = {0};
	int i, ret;

	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(s2mpb02->irq_gpio));

	/* LED_INT */
	ret = s2mpb02_read_reg(s2mpb02->i2c,
			S2MPB02_REG_INT1, &irq_reg[LED_INT]);
	pr_info("%s: led interrupt(0x%02x)\n",
			__func__, irq_reg[LED_INT]);

	pr_debug("%s: irq gpio post-state(0x%02x)\n", __func__,
		gpio_get_value(s2mpb02->irq_gpio));

	/* Apply masking */
	for (i = 0; i < S2MPB02_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~s2mpb02->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < S2MPB02_IRQ_NR; i++) {
		if (irq_reg[s2mpb02_irqs[i].group] & s2mpb02_irqs[i].mask)
			handle_nested_irq(s2mpb02->irq_base + i);
	}

	return IRQ_HANDLED;
}

int s2mpb02_irq_init(struct s2mpb02_dev *s2mpb02)
{
	int i;
	int ret;

	if (!s2mpb02->irq_gpio) {
		pr_warn("%s:%s No interrupt specified.\n",
					MFD_DEV_NAME, __func__);
		s2mpb02->irq_base = 0;
		return 0;
	}

	if (s2mpb02->irq_base < 0) {
		pr_err("%s:%s No interrupt base specified.\n",
					MFD_DEV_NAME, __func__);
		return 0;
	}

	mutex_init(&s2mpb02->irqlock);

	s2mpb02->irq = gpio_to_irq(s2mpb02->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			s2mpb02->irq, s2mpb02->irq_gpio);

	ret = gpio_request(s2mpb02->irq_gpio, "sub_pmic_irq");
	if (ret) {
		pr_err("%s:%s failed requesting gpio %d\n",
			MFD_DEV_NAME, __func__,	s2mpb02->irq_gpio);
		goto err;
	}
	gpio_direction_input(s2mpb02->irq_gpio);
	gpio_free(s2mpb02->irq_gpio);

	/* Mask interrupt sources */
	for (i = 0; i < S2MPB02_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;

		s2mpb02->irq_masks_cur[i] = 0xff;
		s2mpb02->irq_masks_cache[i] = 0xff;

		i2c = s2mpb02->i2c;

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mpb02_mask_reg[i] == S2MPB02_REG_INVALID)
			continue;
		s2mpb02_write_reg(i2c, s2mpb02_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MPB02_IRQ_NR; i++) {
		int cur_irq = i + s2mpb02->irq_base;
		ret = irq_set_chip_data(cur_irq, s2mpb02);
		if (ret) {
			dev_err(s2mpb02->dev,
				"Failed to irq_set_chip_data %d: %d\n",
				s2mpb02->irq, ret);
			goto err;
		}
		irq_set_chip_and_handler(cur_irq, &s2mpb02_irq_chip,
						 handle_edge_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(s2mpb02->irq, NULL, s2mpb02_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "s2mpb02-irq", s2mpb02);
	if (ret) {
		pr_err("%s:%s Failed to request IRQ %d: %d\n", MFD_DEV_NAME,
			__func__, s2mpb02->irq, ret);
		goto err;
	}

	return 0;
err:
	mutex_destroy(&s2mpb02->i2c_lock);
	return ret;
}

void s2mpb02_irq_exit(struct s2mpb02_dev *s2mpb02)
{
	if (s2mpb02->irq)
		free_irq(s2mpb02->irq, s2mpb02);
}

