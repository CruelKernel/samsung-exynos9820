/*
 * s2mps18-irq.c - Interrupt controller support for S2MPS18
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
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
*/

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/samsung/s2mps18.h>
#include <linux/mfd/samsung/s2mps18-private.h>

static const u8 s2mps18_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[PMIC_INT1] = 	S2MPS18_PMIC_REG_INT1M,
	[PMIC_INT2] =	S2MPS18_PMIC_REG_INT2M,
	[PMIC_INT3] = 	S2MPS18_PMIC_REG_INT3M,
	[PMIC_INT4] = 	S2MPS18_PMIC_REG_INT4M,
	[PMIC_INT5] =	S2MPS18_PMIC_REG_INT5M,
	[PMIC_INT6] = 	S2MPS18_PMIC_REG_INT6M,
	[PMIC_INT7] =	S2MPS18_PMIC_REG_INT7M,
};

static struct i2c_client *get_i2c(struct s2mps18_dev *s2mps18,
				enum s2mps18_irq_source src)
{
	switch (src) {
	case PMIC_INT1 ... PMIC_INT7:
		return s2mps18->pmic;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct s2mps18_irq_data {
	int mask;
	enum s2mps18_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct s2mps18_irq_data s2mps18_irqs[] = {
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_PWRONR_INT1,	PMIC_INT1, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_PWRONF_INT1,	PMIC_INT1, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_JIGONBF_INT1,	PMIC_INT1, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_JIGONBR_INT1,	PMIC_INT1, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_ACOKBF_INT1,	PMIC_INT1, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_ACOKBR_INT1,	PMIC_INT1, 1 << 5),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_PWRON1S_INT1,	PMIC_INT1, 1 << 6),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_MRB_INT1,		PMIC_INT1, 1 << 7),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_RTC60S_INT2,	PMIC_INT2, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_RTCA1_INT2,	PMIC_INT2, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_RTCA0_INT2,	PMIC_INT2, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SMPL_INT2,		PMIC_INT2, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_RTC1S_INT2,	PMIC_INT2, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_WTSR_INT2,		PMIC_INT2, 1 << 5),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_WRSTB_INT2,	PMIC_INT2, 1 << 7),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_120C_INT3,		PMIC_INT3, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_140C_INT3,		PMIC_INT3, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_TSD_INT3,		PMIC_INT3, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_ADCDONE_INT3,	PMIC_INT3, 1 << 7),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC0_INT4,		PMIC_INT4, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC1_INT4,		PMIC_INT4, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC2_INT4,		PMIC_INT4, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC3_INT4,		PMIC_INT4, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC4_INT4,		PMIC_INT4, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC5_INT4,		PMIC_INT4, 1 << 5),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC6_INT4,		PMIC_INT4, 1 << 6),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OC7_INT4,		PMIC_INT4, 1 << 7),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB1_INT5,	PMIC_INT5, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB2_INT5,	PMIC_INT5, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB3_INT5,	PMIC_INT5, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB4_INT5,	PMIC_INT5, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB5_INT5,	PMIC_INT5, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB6_INT5,	PMIC_INT5, 1 << 5),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB7_INT5,	PMIC_INT5, 1 << 6),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB8_INT5,	PMIC_INT5, 1 << 7),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB9_INT6,	PMIC_INT6, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB10_INT6,	PMIC_INT6, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB11_INT6,	PMIC_INT6, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB12_INT6,	PMIC_INT6, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB13_INT6,	PMIC_INT6, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPB14_INT6,	PMIC_INT6, 1 << 5),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_OCPBB_INT6,	PMIC_INT6, 1 << 6),

	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SCLDO20_INT7,	PMIC_INT7, 1 << 0),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SCLDO21_INT7,	PMIC_INT7, 1 << 1),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SCLDO31_INT7,	PMIC_INT7, 1 << 2),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SCLDO44_INT7,	PMIC_INT7, 1 << 3),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_SCLDO2_INT7,	PMIC_INT7, 1 << 4),
	DECLARE_IRQ(S2MPS18_PMIC_IRQ_MASKSCLDOINT_INT7,	PMIC_INT7, 1 << 7),
};

static void s2mps18_irq_lock(struct irq_data *data)
{
	struct s2mps18_dev *s2mps18 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mps18->irqlock);
}

static void s2mps18_irq_sync_unlock(struct irq_data *data)
{
	struct s2mps18_dev *s2mps18 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MPS18_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mps18_mask_reg[i];
		struct i2c_client *i2c = get_i2c(s2mps18, i);

		if (mask_reg == S2MPS18_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		s2mps18->irq_masks_cache[i] = s2mps18->irq_masks_cur[i];

		s2mps18_write_reg(i2c, s2mps18_mask_reg[i],
				s2mps18->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mps18->irqlock);
}

static const inline struct s2mps18_irq_data *
irq_to_s2mps18_irq(struct s2mps18_dev *s2mps18, int irq)
{
	return &s2mps18_irqs[irq - s2mps18->irq_base];
}

static void s2mps18_irq_mask(struct irq_data *data)
{
	struct s2mps18_dev *s2mps18 = irq_get_chip_data(data->irq);
	const struct s2mps18_irq_data *irq_data =
	    irq_to_s2mps18_irq(s2mps18, data->irq);

	if (irq_data->group >= S2MPS18_IRQ_GROUP_NR)
		return;

	s2mps18->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void s2mps18_irq_unmask(struct irq_data *data)
{
	struct s2mps18_dev *s2mps18 = irq_get_chip_data(data->irq);
	const struct s2mps18_irq_data *irq_data =
	    irq_to_s2mps18_irq(s2mps18, data->irq);

	if (irq_data->group >= S2MPS18_IRQ_GROUP_NR)
		return;

	s2mps18->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip s2mps18_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= s2mps18_irq_lock,
	.irq_bus_sync_unlock	= s2mps18_irq_sync_unlock,
	.irq_mask		= s2mps18_irq_mask,
	.irq_unmask		= s2mps18_irq_unmask,
};

static irqreturn_t s2mps18_irq_thread(int irq, void *data)
{
	struct s2mps18_dev *s2mps18 = data;
	u8 irq_reg[S2MPS18_IRQ_GROUP_NR] = {0};
	u8 irq_src;
	int i, ret;

	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(s2mps18->irq_gpio));

	ret = s2mps18_read_reg(s2mps18->i2c,
					S2MPS18_PMIC_REG_INTSRC, &irq_src);
	if (ret) {
		pr_err("%s:%s Failed to read interrupt source: %d\n",
			MFD_DEV_NAME, __func__, ret);
		return IRQ_NONE;
	}

	pr_info("%s: interrupt source(0x%02x)\n", __func__, irq_src);

	if (irq_src & S2MPS18_IRQSRC_PMIC) {
		/* PMIC_INT */
		ret = s2mps18_bulk_read(s2mps18->pmic, S2MPS18_PMIC_REG_INT1,
				S2MPS18_NUM_IRQ_PMIC_REGS, &irq_reg[PMIC_INT1]);
		if (ret) {
			pr_err("%s:%s Failed to read pmic interrupt: %d\n",
				MFD_DEV_NAME, __func__, ret);
			return IRQ_NONE;
		}

		pr_info("%s: pmic interrupt(0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
			 __func__, irq_reg[PMIC_INT1], irq_reg[PMIC_INT2], irq_reg[PMIC_INT3], 
			irq_reg[PMIC_INT4], irq_reg[PMIC_INT5], irq_reg[PMIC_INT6], irq_reg[PMIC_INT7]);
	}

	/* Apply masking */
	for (i = 0; i < S2MPS18_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~s2mps18->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < S2MPS18_IRQ_NR; i++) {
		if (irq_reg[s2mps18_irqs[i].group] & s2mps18_irqs[i].mask)
			handle_nested_irq(s2mps18->irq_base + i);
	}

	return IRQ_HANDLED;
}

int s2mps18_irq_init(struct s2mps18_dev *s2mps18)
{
	int i;
	int ret;
	u8 i2c_data;
	int cur_irq;

	if (!s2mps18->irq_gpio) {
		dev_warn(s2mps18->dev, "No interrupt specified.\n");
		s2mps18->irq_base = 0;
		return 0;
	}

	if (!s2mps18->irq_base) {
		dev_err(s2mps18->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&s2mps18->irqlock);

	s2mps18->irq = gpio_to_irq(s2mps18->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			s2mps18->irq, s2mps18->irq_gpio);

	ret = gpio_request(s2mps18->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(s2mps18->dev, "%s: failed requesting gpio %d\n",
			__func__, s2mps18->irq_gpio);
		return ret;
	}
	gpio_direction_input(s2mps18->irq_gpio);
	gpio_free(s2mps18->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < S2MPS18_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;

		s2mps18->irq_masks_cur[i] = 0xff;
		s2mps18->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(s2mps18, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mps18_mask_reg[i] == S2MPS18_REG_INVALID)
			continue;

		s2mps18_write_reg(i2c, s2mps18_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MPS18_IRQ_NR; i++) {
		cur_irq = i + s2mps18->irq_base;
		irq_set_chip_data(cur_irq, s2mps18);
		irq_set_chip_and_handler(cur_irq, &s2mps18_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_INTSRC_MASK, 0xff);
	/* Unmask s2mps18 interrupt */
	ret = s2mps18_read_reg(s2mps18->i2c, S2MPS18_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read intsrc mask reg\n",
					 MFD_DEV_NAME, __func__);
		return ret;
	}

	i2c_data &= ~(S2MPS18_IRQSRC_PMIC);	/* Unmask pmic interrupt */
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	pr_info("%s:%s s2mps18_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	ret = request_threaded_irq(s2mps18->irq, NULL, s2mps18_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "s2mps18-irq", s2mps18);

	if (ret) {
		dev_err(s2mps18->dev, "Failed to request IRQ %d: %d\n",
			s2mps18->irq, ret);
		return ret;
	}

	return 0;
}

void s2mps18_irq_exit(struct s2mps18_dev *s2mps18)
{
	if (s2mps18->irq)
		free_irq(s2mps18->irq, s2mps18);
}
