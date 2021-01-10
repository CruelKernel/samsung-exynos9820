/*
 * max77705-irq.c - Interrupt controller support for MAX77705
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This driver is based on max77705-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>

static const u8 max77705_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[SYS_INT] = MAX77705_PMIC_REG_SYSTEM_INT_MASK,
	[CHG_INT] = MAX77705_CHG_REG_INT_MASK,
	[FUEL_INT] = MAX77705_REG_INVALID,
	[USBC_INT] = MAX77705_USBC_REG_UIC_INT_M,
	[CC_INT] = MAX77705_USBC_REG_CC_INT_M,
	[PD_INT] = MAX77705_USBC_REG_PD_INT_M,
	[VDM_INT] = MAX77705_USBC_REG_VDM_INT_M,
	[VIR_INT] = MAX77705_REG_INVALID,
};

static struct i2c_client *get_i2c(struct max77705_dev *max77705,
				enum max77705_irq_source src)
{
	switch (src) {
	case SYS_INT:
		return max77705->i2c;
	case FUEL_INT:
		return max77705->fuelgauge;
	case CHG_INT:
		return max77705->charger;
	case USBC_INT:
	case CC_INT:
	case PD_INT:
	case VDM_INT:
	case VIR_INT:
		return max77705->muic;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct max77705_irq_data {
	int mask;
	enum max77705_irq_source group;
};

static const struct max77705_irq_data max77705_irqs[] = {
	[MAX77705_SYSTEM_IRQ_BSTEN_INT] = { .group = SYS_INT, .mask = 1 << 3 },
	[MAX77705_SYSTEM_IRQ_SYSUVLO_INT] = { .group = SYS_INT, .mask = 1 << 4 },
	[MAX77705_SYSTEM_IRQ_SYSOVLO_INT] = { .group = SYS_INT, .mask = 1 << 5 },
	[MAX77705_SYSTEM_IRQ_TSHDN_INT] = { .group = SYS_INT, .mask = 1 << 6 },
	[MAX77705_SYSTEM_IRQ_TM_INT] = { .group = SYS_INT, .mask = 1 << 7 },

	[MAX77705_CHG_IRQ_BYP_I] = { .group = CHG_INT, .mask = 1 << 0 },
	[MAX77705_CHG_IRQ_BATP_I] = { .group = CHG_INT, .mask = 1 << 2 },
	[MAX77705_CHG_IRQ_BAT_I] = { .group = CHG_INT, .mask = 1 << 3 },
	[MAX77705_CHG_IRQ_CHG_I] = { .group = CHG_INT, .mask = 1 << 4 },
	[MAX77705_CHG_IRQ_WCIN_I] = { .group = CHG_INT, .mask = 1 << 5 },
	[MAX77705_CHG_IRQ_CHGIN_I] = { .group = CHG_INT, .mask = 1 << 6 },
	[MAX77705_CHG_IRQ_AICL_I] = { .group = CHG_INT, .mask = 1 << 7 },

	[MAX77705_FG_IRQ_ALERT] = { .group = FUEL_INT, .mask = 1 << 1 },

	[MAX77705_USBC_IRQ_APC_INT] = { .group = USBC_INT, .mask = 1 << 7 },
	[MAX77705_USBC_IRQ_SYSM_INT] = { .group = USBC_INT, .mask = 1 << 6 },
	[MAX77705_USBC_IRQ_VBUS_INT] = { .group = USBC_INT, .mask = 1 << 5 },
	[MAX77705_USBC_IRQ_VBADC_INT] = { .group = USBC_INT, .mask = 1 << 4 },
	[MAX77705_USBC_IRQ_DCD_INT] = { .group = USBC_INT, .mask = 1 << 3 },
	[MAX77705_USBC_IRQ_FAKVB_INT] = { .group = USBC_INT, .mask = 1 << 2 },
	[MAX77705_USBC_IRQ_CHGT_INT] = { .group = USBC_INT, .mask = 1 << 1 },
	[MAX77705_USBC_IRQ_UIDADC_INT] = { .group = USBC_INT, .mask = 1 << 0 },

	[MAX77705_CC_IRQ_VCONNCOP_INT] = { .group = CC_INT, .mask = 1 << 7 },
	[MAX77705_CC_IRQ_VSAFE0V_INT] = { .group = CC_INT, .mask = 1 << 6 },
	[MAX77705_CC_IRQ_DETABRT_INT] = { .group = CC_INT, .mask = 1 << 5 },
	[MAX77705_CC_IRQ_VCONNSC_INT] = { .group = CC_INT, .mask = 1 << 4 },
	[MAX77705_CC_IRQ_CCPINSTAT_INT] = { .group = CC_INT, .mask = 1 << 3 },
	[MAX77705_CC_IRQ_CCISTAT_INT] = { .group = CC_INT, .mask = 1 << 2 },
	[MAX77705_CC_IRQ_CCVCNSTAT_INT] = { .group = CC_INT, .mask = 1 << 1 },
	[MAX77705_CC_IRQ_CCSTAT_INT] = { .group = CC_INT, .mask = 1 << 0 },

	[MAX77705_PD_IRQ_PDMSG_INT] = { .group = PD_INT, .mask = 1 << 7 },
	[MAX77705_PD_IRQ_PS_RDY_INT] = { .group = PD_INT, .mask = 1 << 6 },
	[MAX77705_PD_IRQ_DATAROLE_INT] = { .group = PD_INT, .mask = 1 << 5 },
	[MAX77705_IRQ_VDM_ATTENTION_INT] = { .group = PD_INT, .mask = 1 << 4 },
	[MAX77705_IRQ_VDM_DP_CONFIGURE_INT] = { .group = PD_INT, .mask = 1 << 3 },
	[MAX77705_IRQ_VDM_DP_STATUS_UPDATE_INT] = { .group = PD_INT, .mask = 1 << 2 },
	[MAX77705_PD_IRQ_SSACCI_INT] = { .group = PD_INT, .mask = 1 << 1 },
	[MAX77705_PD_IRQ_FCTIDI_INT] = { .group = PD_INT, .mask = 1 << 0 },

	[MAX77705_IRQ_VDM_DISCOVER_ID_INT] = { .group = VDM_INT, .mask = 1 << 0 },
	[MAX77705_IRQ_VDM_DISCOVER_SVIDS_INT] = { .group = VDM_INT, .mask = 1 << 1 },
	[MAX77705_IRQ_VDM_DISCOVER_MODES_INT] = { .group = VDM_INT, .mask = 1 << 2 },
	[MAX77705_IRQ_VDM_ENTER_MODE_INT] = { .group = VDM_INT, .mask = 1 << 3 },

	[MAX77705_VIR_IRQ_ALTERROR_INT] = { .group = VIR_INT, .mask = 1 << 0 },
};

static void max77705_irq_lock(struct irq_data *data)
{
	struct max77705_dev *max77705 = irq_get_chip_data(data->irq);

	mutex_lock(&max77705->irqlock);
}

static void max77705_irq_sync_unlock(struct irq_data *data)
{
	struct max77705_dev *max77705 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < MAX77705_IRQ_GROUP_NR; i++) {
		u8 mask_reg = max77705_mask_reg[i];
		struct i2c_client *i2c = get_i2c(max77705, i);

		if (mask_reg == MAX77705_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		max77705->irq_masks_cache[i] = max77705->irq_masks_cur[i];

		max77705_write_reg(i2c, max77705_mask_reg[i],
				max77705->irq_masks_cur[i]);
	}

	mutex_unlock(&max77705->irqlock);
}

static const inline struct max77705_irq_data *
irq_to_max77705_irq(struct max77705_dev *max77705, int irq)
{
	return &max77705_irqs[irq - max77705->irq_base];
}

static void max77705_irq_mask(struct irq_data *data)
{
	struct max77705_dev *max77705 = irq_get_chip_data(data->irq);
	const struct max77705_irq_data *irq_data =
	    irq_to_max77705_irq(max77705, data->irq);

	if (irq_data->group >= MAX77705_IRQ_GROUP_NR)
		return;

	max77705->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void max77705_irq_unmask(struct irq_data *data)
{
	struct max77705_dev *max77705 = irq_get_chip_data(data->irq);
	const struct max77705_irq_data *irq_data =
	    irq_to_max77705_irq(max77705, data->irq);

	if (irq_data->group >= MAX77705_IRQ_GROUP_NR)
		return;

	max77705->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static void max77705_irq_disable(struct irq_data *data)
{
	max77705_irq_mask(data);
}

static struct irq_chip max77705_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= max77705_irq_lock,
	.irq_bus_sync_unlock	= max77705_irq_sync_unlock,
	.irq_mask		= max77705_irq_mask,
	.irq_unmask		= max77705_irq_unmask,
	.irq_disable            = max77705_irq_disable,
};

#define VB_LOW 0

static irqreturn_t max77705_irq_thread(int irq, void *data)
{
	struct max77705_dev *max77705 = data;
	u8 irq_reg[MAX77705_IRQ_GROUP_NR] = {0};
	u8 irq_src;
	int i, ret;
	u8 irq_vdm_mask = 0x0;
	u8 dummy[2] = {0, }; /* for pass1 intr reg clear issue */
	u8 dump_reg[10] = {0, };
	u8 pmic_rev = max77705->pmic_rev;
	u8 reg_data;
	u8 cc_status0 = 0;
	u8 cc_status1 = 0;
	u8 bc_status0 = 0;
	u8 ccstat = 0;
	u8 vbvolt = 0;
	u8 pre_ccstati = 0;
	u8 ic_alt_mode = 0;

	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(max77705->irq_gpio));

	ret = max77705_read_reg(max77705->i2c,
					MAX77705_PMIC_REG_INTSRC, &irq_src);
	if (ret) {
		pr_err("%s:%s Failed to read interrupt source: %d\n",
			MFD_DEV_NAME, __func__, ret);
		return IRQ_NONE;
	}

	pr_info("%s:%s: irq[%d] %d/%d/%d irq_src=0x%02x pmic_rev=0x%02x\n", MFD_DEV_NAME, __func__,
		irq, max77705->irq, max77705->irq_base, max77705->irq_gpio, irq_src, pmic_rev);

	if (irq_src & MAX77705_IRQSRC_CHG) {
		/* CHG_INT */
		switch (pmic_rev) {
		case MAX77705_PASS1:
			ret = max77705_bulk_read(max77705->charger, MAX77705_CHG_REG_INT - 1,
					2, dummy);
			irq_reg[CHG_INT] = dummy[1];
			break;
		case MAX77705_PASS2:
		case MAX77705_PASS3:
		case MAX77705_PASS4:
		case MAX77705_PASS5:
			ret = max77705_read_reg(max77705->charger, MAX77705_CHG_REG_INT,
					&irq_reg[CHG_INT]);
			break;
		default:
			pr_err("%s: PMIC_REVISION(SRC_CHG) isn't valid\n", __func__);
			break;
		}
		pr_debug("%s: charger interrupt(0x%02x)\n",
				__func__, irq_reg[CHG_INT]);
		/* mask chgin to prevent chgin infinite interrupt
		 * chgin is unmasked chgin isr
		 */
		if (irq_reg[CHG_INT] &
				max77705_irqs[MAX77705_CHG_IRQ_CHGIN_I].mask) {
			max77705_read_reg(max77705->charger,
				MAX77705_CHG_REG_INT_MASK, &reg_data);
			reg_data |= (1 << 6);
			max77705_write_reg(max77705->charger,
				MAX77705_CHG_REG_INT_MASK, reg_data);
		}
	}


	if (irq_src & MAX77705_IRQSRC_FG) {
		pr_debug("[%s] fuelgauge interrupt\n", __func__);
		pr_debug("[%s]IRQ_BASE(%d), NESTED_IRQ(%d)\n",
			__func__, max77705->irq_base, max77705->irq_base + MAX77705_FG_IRQ_ALERT);
		handle_nested_irq(max77705->irq_base + MAX77705_FG_IRQ_ALERT);
		return IRQ_HANDLED;
	}

	if (irq_src & MAX77705_IRQSRC_TOP) {
		/* SYS_INT */
		switch (pmic_rev) {
		case MAX77705_PASS1:
			ret = max77705_bulk_read(max77705->i2c, MAX77705_PMIC_REG_SYSTEM_INT - 1,
					2, dummy);
			irq_reg[SYS_INT] = dummy[1];
			break;
		case MAX77705_PASS2:
		case MAX77705_PASS3:
		case MAX77705_PASS4:
		case MAX77705_PASS5:
			ret = max77705_read_reg(max77705->i2c, MAX77705_PMIC_REG_SYSTEM_INT,
					&irq_reg[SYS_INT]);
			break;
		default:
			pr_err("%s: PMIC_REVISION(SRC_TOP) isn't valid\n", __func__);
			break;
		}
		pr_debug("%s: topsys interrupt(0x%02x)\n", __func__, irq_reg[SYS_INT]);
	}

	if ((irq_src & MAX77705_IRQSRC_USBC) && max77705->cc_booting_complete) {
		/* USBC INT */
		switch (pmic_rev) {
		case MAX77705_PASS1:
			ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_UIC_INT - 1,
					2, dummy);
			irq_reg[USBC_INT] = dummy[1];
			ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_CC_INT - 1,
					2, dummy);
			irq_reg[USBC_INT] |= dummy[0];
			irq_reg[CC_INT] = dummy[1];
			ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_PD_INT - 1,
					2, dummy);
			irq_reg[CC_INT] |= dummy[0];
			irq_reg[PD_INT] = dummy[1];
			ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_VDM_INT_M,
					1, &irq_vdm_mask);
			if (irq_vdm_mask == 0x0) {
				ret = max77705_bulk_read(max77705->muic,
						MAX77705_USBC_REG_VDM_INT - 1, 2, dummy);
				irq_reg[PD_INT] |= dummy[0];
				irq_reg[VDM_INT] = dummy[1];
			}
			break;
		case MAX77705_PASS2:
		case MAX77705_PASS3:
		case MAX77705_PASS4:
		case MAX77705_PASS5:
			ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_UIC_INT,
					4, &irq_reg[USBC_INT]);
			ret = max77705_read_reg(max77705->muic, MAX77705_USBC_REG_VDM_INT_M,
					&irq_vdm_mask);
			if (irq_reg[USBC_INT] & BIT_VBUSDetI) {
				ret = max77705_read_reg(max77705->muic, REG_BC_STATUS, &bc_status0);
				ret = max77705_read_reg(max77705->muic, REG_CC_STATUS0, &cc_status0);
				vbvolt = (bc_status0 & BIT_VBUSDet) >> FFS(BIT_VBUSDet);
				ccstat = (cc_status0 & BIT_CCStat) >> FFS(BIT_CCStat);
				if (cc_No_Connection == ccstat && vbvolt == VB_LOW) {
					pre_ccstati = irq_reg[CC_INT];
					irq_reg[CC_INT] |= 0x1;
					pr_info("[MAX77705] set the cc_stat int [work-around] :%x, %x\n",
						pre_ccstati,irq_reg[CC_INT]);
				}
			}
			break;
		default:
			pr_err("%s: PMIC_REVISION(SRC_MUIC) isn't valid\n", __func__);
			break;
		}
		ret = max77705_bulk_read(max77705->muic, MAX77705_USBC_REG_USBC_STATUS1,
				8, dump_reg);
		pr_info("[MAX77705] irq_reg, complete [%x], %x, %x, %x, %x, %x\n", max77705->cc_booting_complete,
			irq_reg[USBC_INT], irq_reg[CC_INT], irq_reg[PD_INT], irq_reg[VDM_INT], irq_vdm_mask);
		pr_info("[MAX77705] dump_reg, %x, %x, %x, %x, %x, %x, %x, %x\n", dump_reg[0], dump_reg[1],
			dump_reg[2], dump_reg[3], dump_reg[4], dump_reg[5], dump_reg[6], dump_reg[7]);
	}

	if (max77705->cc_booting_complete) {
		max77705_read_reg(max77705->muic, REG_CC_STATUS1, &cc_status1);
		ic_alt_mode = (cc_status1 & BIT_Altmode) >> FFS(BIT_Altmode);
		if (!ic_alt_mode && max77705->set_altmode)
			irq_reg[VIR_INT] |= (1 << 0);
		pr_info("%s ic_alt_mode=%d\n", __func__, ic_alt_mode);

		if (irq_reg[PD_INT] & BIT_PDMsg) {
			if (dump_reg[6] == Sink_PD_PSRdy_received
					|| dump_reg[6] == SRC_CAP_RECEIVED) {
				if (max77705->check_pdmsg)
					max77705->check_pdmsg(max77705->usbc_data, dump_reg[6]);
			}
		}
	}

	/* Apply masking */
	for (i = 0; i < MAX77705_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~max77705->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < MAX77705_IRQ_NR; i++) {
		if (irq_reg[max77705_irqs[i].group] & max77705_irqs[i].mask)
			handle_nested_irq(max77705->irq_base + i);
	}

	return IRQ_HANDLED;
}

int max77705_irq_init(struct max77705_dev *max77705)
{
	int i;
	int ret;
	u8 i2c_data;
	int cur_irq;

	if (!max77705->irq_gpio) {
		dev_warn(max77705->dev, "No interrupt specified.\n");
		max77705->irq_base = 0;
		return 0;
	}

	if (!max77705->irq_base) {
		dev_err(max77705->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&max77705->irqlock);

	max77705->irq = gpio_to_irq(max77705->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			max77705->irq, max77705->irq_gpio);

	ret = gpio_request(max77705->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(max77705->dev, "%s: failed requesting gpio %d\n",
			__func__, max77705->irq_gpio);
		return ret;
	}
	gpio_direction_input(max77705->irq_gpio);
	gpio_free(max77705->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < MAX77705_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;
		/* MUIC IRQ  0:MASK 1:NOT MASK => NOT USE */
		/* Other IRQ 1:MASK 0:NOT MASK */
		max77705->irq_masks_cur[i] = 0xff;
		max77705->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(max77705, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (max77705_mask_reg[i] == MAX77705_REG_INVALID)
			continue;
		max77705_write_reg(i2c, max77705_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < MAX77705_IRQ_NR; i++) {
		cur_irq = i + max77705->irq_base;
		irq_set_chip_data(cur_irq, max77705);
		irq_set_chip_and_handler(cur_irq, &max77705_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}



	/* Unmask max77705 interrupt */
	ret = max77705_read_reg(max77705->i2c, MAX77705_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read muic reg\n", MFD_DEV_NAME, __func__);
		return ret;
	}
	i2c_data |= 0xF;	/* mask muic interrupt */

	max77705_write_reg(max77705->i2c, MAX77705_PMIC_REG_INTSRC_MASK,
			   i2c_data);


	ret = request_threaded_irq(max77705->irq, NULL, max77705_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "max77705-irq", max77705);
	if (ret) {
		dev_err(max77705->dev, "Failed to request IRQ %d: %d\n",
			max77705->irq, ret);
		return ret;
	}


	/* Unmask max77705 interrupt */
	ret = max77705_read_reg(max77705->i2c, MAX77705_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read muic reg\n", MFD_DEV_NAME, __func__);
		return ret;
	}

	i2c_data &= ~(MAX77705_IRQSRC_CHG);	/* Unmask charger interrupt */
//	i2c_data &= ~(MAX77705_IRQSRC_FG);      /* Unmask fg interrupt */
	i2c_data |= MAX77705_IRQSRC_USBC;	/* mask usbc interrupt */

	max77705_write_reg(max77705->i2c, MAX77705_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	pr_info("%s:%s max77705_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	return 0;
}

void max77705_irq_exit(struct max77705_dev *max77705)
{
	if (max77705->irq)
		free_irq(max77705->irq, max77705);
}
