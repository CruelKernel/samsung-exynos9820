/*
 * s2mu004-irq.c - Interrupt controller support for s2mu004
 *
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
 *
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
 * This driver is based on s2mu004-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/samsung/s2mu004.h>
#include <linux/mfd/samsung/s2mu004-private.h>

static const u8 s2mu004_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[CHG_INT1] = S2MU004_REG_SC_INT1_MASK,
	[CHG_INT2] = S2MU004_REG_SC_INT2_MASK,
#if defined(CONFIG_HV_MUIC_S2MU004_AFC)
	[AFC_INT] = S2MU004_REG_AFC_INT_MASK,
#endif
	[MUIC_INT1] = S2MU004_REG_MUIC_INT1_MASK,
	[MUIC_INT2] = S2MU004_REG_MUIC_INT2_MASK,
};

struct s2mu004_irq_data {
	int mask;
	enum s2mu004_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }

static const struct s2mu004_irq_data s2mu004_irqs[] = {
	DECLARE_IRQ(S2MU004_CHG1_IRQ_SYS,	CHG_INT1,	1 << 0),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_Poor_CHG,	CHG_INT1,	1 << 1),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_CHG_Fault,	CHG_INT1,	1 << 2),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_CHG_RSTART, CHG_INT1,	1 << 3),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_DONE,	CHG_INT1,	1 << 4),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_TOP_OFF,	CHG_INT1,	1 << 5),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_WCIN,	CHG_INT1,	1 << 6),
	DECLARE_IRQ(S2MU004_CHG1_IRQ_CHGIN,	CHG_INT1,	1 << 7),

	DECLARE_IRQ(S2MU004_CHG2_IRQ_ICR,	CHG_INT2,	1 << 0),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_IVR,	CHG_INT2,	1 << 1),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_AICL,	CHG_INT2,	1 << 2),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_TX_Fault,	CHG_INT2,	1 << 3),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_OTG_Fault,	CHG_INT2,	1 << 4),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_DET_BAT,	CHG_INT2,	1 << 5),
	DECLARE_IRQ(S2MU004_CHG2_IRQ_BAT,	CHG_INT2,	1 << 6),

#if defined(CONFIG_HV_MUIC_S2MU004_AFC)
	DECLARE_IRQ(S2MU004_AFC_IRQ_VbADC,	AFC_INT,	1 << 0),
	DECLARE_IRQ(S2MU004_AFC_IRQ_VDNMon,	AFC_INT,	1 << 1),
	DECLARE_IRQ(S2MU004_AFC_IRQ_DNRes,	AFC_INT,	1 << 2),
	DECLARE_IRQ(S2MU004_AFC_IRQ_MPNack,	AFC_INT,	1 << 3),
	DECLARE_IRQ(S2MU004_AFC_IRQ_MRxTrf,	AFC_INT,	1 << 5),
	DECLARE_IRQ(S2MU004_AFC_IRQ_MRxPerr,	AFC_INT,	1 << 6),
	DECLARE_IRQ(S2MU004_AFC_IRQ_MRxRdy,	AFC_INT,	1 << 7),
#endif
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_ATTATCH,	MUIC_INT1,	1 << 0),
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_DETACH,	MUIC_INT1,	1 << 1),
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_KP,	MUIC_INT1,	1 << 2),
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_LKP,	MUIC_INT1,	1 << 3),
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_LKR,	MUIC_INT1,	1 << 4),
	DECLARE_IRQ(S2MU004_MUIC_IRQ1_RID_CHG,	MUIC_INT1,	1 << 5),

	DECLARE_IRQ(S2MU004_MUIC_IRQ2_VBUS_ON,		MUIC_INT2, 1 << 0),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_RSVD_ATTACH,	MUIC_INT2, 1 << 1),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_ADC_CHANGE,	MUIC_INT2, 1 << 2),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_STUCK,		MUIC_INT2, 1 << 3),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_STUCKRCV,		MUIC_INT2, 1 << 4),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_MHDL,		MUIC_INT2, 1 << 5),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_AV_CHARGE,	MUIC_INT2, 1 << 6),
	DECLARE_IRQ(S2MU004_MUIC_IRQ2_VBUS_OFF,		MUIC_INT2, 1 << 7),
};

static void s2mu004_irq_lock(struct irq_data *data)
{
	struct s2mu004_dev *s2mu004 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mu004->irqlock);
}

static void s2mu004_irq_sync_unlock(struct irq_data *data)
{
	struct s2mu004_dev *s2mu004 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MU004_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mu004_mask_reg[i];
		struct i2c_client *i2c = s2mu004->i2c;

		if (mask_reg == S2MU004_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		s2mu004->irq_masks_cache[i] = s2mu004->irq_masks_cur[i];

		s2mu004_write_reg(i2c, s2mu004_mask_reg[i],
				s2mu004->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mu004->irqlock);
}

static const inline struct s2mu004_irq_data *
irq_to_s2mu004_irq(struct s2mu004_dev *s2mu004, int irq)
{
	return &s2mu004_irqs[irq - s2mu004->irq_base];
}

static void s2mu004_irq_mask(struct irq_data *data)
{
	struct s2mu004_dev *s2mu004 = irq_get_chip_data(data->irq);
	const struct s2mu004_irq_data *irq_data =
	    irq_to_s2mu004_irq(s2mu004, data->irq);

	if (irq_data->group >= S2MU004_IRQ_GROUP_NR)
		return;

	s2mu004->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void s2mu004_irq_unmask(struct irq_data *data)
{
	struct s2mu004_dev *s2mu004 = irq_get_chip_data(data->irq);
	const struct s2mu004_irq_data *irq_data =
	    irq_to_s2mu004_irq(s2mu004, data->irq);

	if (irq_data->group >= S2MU004_IRQ_GROUP_NR)
		return;

	s2mu004->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip s2mu004_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock	= s2mu004_irq_lock,
	.irq_bus_sync_unlock	= s2mu004_irq_sync_unlock,
	.irq_mask		= s2mu004_irq_mask,
	.irq_unmask		= s2mu004_irq_unmask,
};

static irqreturn_t s2mu004_irq_thread(int irq, void *data)
{
	struct s2mu004_dev *s2mu004 = data;
	u8 irq_reg[S2MU004_IRQ_GROUP_NR] = {0};
	int i, ret;
	u8 temp, temp_2;
	u8 temp_vdadc;

	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
			gpio_get_value(s2mu004->irq_gpio));

	/* CHG_INT1 ~ INT2 */
	ret = s2mu004_read_reg(s2mu004->i2c, S2MU004_REG_SC_INT1,
				&irq_reg[CHG_INT1]);
	pr_info("%s: charger interrupt(0x%02x)\n",
			__func__, irq_reg[CHG_INT1]);

	ret = s2mu004_read_reg(s2mu004->i2c, S2MU004_REG_SC_INT2,
				&irq_reg[CHG_INT2]);
	pr_info("%s: charger interrupt(0x%02x)\n",
			__func__, irq_reg[CHG_INT2]);

	/* AFC_INT */
#if defined(CONFIG_HV_MUIC_S2MU004_AFC)
	ret = s2mu004_read_reg(s2mu004->i2c, S2MU004_REG_AFC_INT,
				&irq_reg[AFC_INT]);
	pr_info("%s: AFC interrupt(0x%02x)\n",
			__func__, irq_reg[AFC_INT]);
#endif
	ret = s2mu004_read_reg(s2mu004->i2c, 0x48,
				&temp_vdadc);
	pr_info("%s: 0x48 (0x%02x)\n",
			__func__, temp_vdadc);
	/* MUIC INT1 ~ INT2 */
	ret = s2mu004_bulk_read(s2mu004->i2c, S2MU004_REG_MUIC_INT1,
				S2MU004_NUM_IRQ_MUIC_REGS, &irq_reg[MUIC_INT1]);
	pr_info("%s: muic interrupt(0x%02x, 0x%02x)\n", __func__,
			irq_reg[MUIC_INT1], irq_reg[MUIC_INT2]);

	if (s2mu004->pmic_rev == 0) {
		s2mu004_read_reg(s2mu004->i2c, S2MU004_REG_MUIC_ADC, &temp);
		temp &= 0x1F;
		/* checking VBUS_WAKEUP bit of R(0x61) */
		s2mu004_read_reg(s2mu004->i2c, 0x69, &temp_2);
		if ((temp_2 & 0x02) && (temp != 0x18)
			&& (temp != 0x19) && (temp != 0x1C) && (temp != 0x1D))
			s2mu004_update_reg(s2mu004->i2c, 0x89, 0x01, 0x03);
		if (irq_reg[MUIC_INT2] & 0x80)
			s2mu004_update_reg(s2mu004->i2c, 0x89, 0x03, 0x03);
	}

	/* Apply masking */
	for (i = 0; i < S2MU004_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~s2mu004->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < S2MU004_IRQ_NR; i++) {
		if (irq_reg[s2mu004_irqs[i].group] & s2mu004_irqs[i].mask)
			handle_nested_irq(s2mu004->irq_base + i);
	}

	return IRQ_HANDLED;
}
static int irq_is_enable = true;
int s2mu004_irq_init(struct s2mu004_dev *s2mu004)
{
	int i;
	int ret;
	struct i2c_client *i2c = s2mu004->i2c;
	int cur_irq;

	if (!s2mu004->irq_gpio) {
		dev_warn(s2mu004->dev, "No interrupt specified.\n");
		s2mu004->irq_base = 0;
		return 0;
	}

	if (!s2mu004->irq_base) {
		dev_err(s2mu004->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&s2mu004->irqlock);

	s2mu004->irq = gpio_to_irq(s2mu004->irq_gpio);
	pr_err("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			s2mu004->irq, s2mu004->irq_gpio);

	ret = gpio_request(s2mu004->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(s2mu004->dev, "%s: failed requesting gpio %d\n",
			__func__, s2mu004->irq_gpio);
		return ret;
	}
	gpio_direction_input(s2mu004->irq_gpio);
	gpio_free(s2mu004->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < S2MU004_IRQ_GROUP_NR; i++) {

		s2mu004->irq_masks_cur[i] = 0xff;
		s2mu004->irq_masks_cache[i] = 0xff;

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mu004_mask_reg[i] == S2MU004_REG_INVALID)
			continue;
		s2mu004_write_reg(i2c, s2mu004_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MU004_IRQ_NR; i++) {
		cur_irq = 0;
		cur_irq = i + s2mu004->irq_base;
		irq_set_chip_data(cur_irq, s2mu004);
		irq_set_chip_and_handler(cur_irq, &s2mu004_irq_chip,
						handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	if (irq_is_enable) {
		ret = request_threaded_irq(s2mu004->irq, NULL,
				s2mu004_irq_thread,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				"s2mu004-irq", s2mu004);
	}

	if (ret) {
		dev_err(s2mu004->dev, "Failed to request IRQ %d: %d\n",
			s2mu004->irq, ret);
		return ret;
	}

	return 0;
}

void s2mu004_irq_exit(struct s2mu004_dev *s2mu004)
{
	if (s2mu004->irq)
		free_irq(s2mu004->irq, s2mu004);
}
