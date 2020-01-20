/*
 * s2mpb02.h - Driver for the Samsung s2mpb02
 *
 *  Copyright (C) 2014 Samsung Electrnoics
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
 *
 * S2MPB02 has Flash LED devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __S2MPB02_H__
#define __S2MPB02_H__

#include <linux/i2c.h>

#define MFD_DEV_NAME "s2mpb02"

#define S2MPB02_I2C_ADDR (0xB2 >> 1)
#define S2MPB02_REG_INVALID (0xff)

#define S2MPB02_PMIC_REV(iodev)	(iodev)->rev_num

enum s2mpb02_types {
	TYPE_S2MPB02,
};

enum s2mpb02_irq_source {
	LED_INT = 0,
	S2MPB02_IRQ_GROUP_NR,
};

enum s2mpb02_irq {
	/* FLASH */
	S2MPB02_LED_IRQ_IRLED_END,

	S2MPB02_IRQ_NR,
};

struct s2mpb02_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0xB2; PMIC, Flash LED */
	struct mutex i2c_lock;

	int type;
	u8 rev_num; /* pmic Rev */
	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[S2MPB02_IRQ_GROUP_NR];
	int irq_masks_cache[S2MPB02_IRQ_GROUP_NR];

	struct s2mpb02_platform_data *pdata;
};

#ifdef CONFIG_LEDS_S2MPB02
struct s2mpb02_led_platform_data;
#endif

struct s2mpb02_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct s2mpb02_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;

	int num_regulators;
	struct s2mpb02_regulator_data *regulators;
#ifdef CONFIG_LEDS_S2MPB02
	/* led (flash/torch) data */
	struct s2mpb02_led_platform_data *led_data;
#endif
};

extern int s2mpb02_irq_init(struct s2mpb02_dev *s2mpb02);
extern void s2mpb02_irq_exit(struct s2mpb02_dev *s2mpb02);
extern int s2mpb02_irq_resume(struct s2mpb02_dev *s2mpb02);

/* S2MPB02 shared i2c API function */
extern int s2mpb02_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2mpb02_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mpb02_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2mpb02_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mpb02_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#endif /* __S2MPB02_H__ */

