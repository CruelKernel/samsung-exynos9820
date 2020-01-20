/*
 * da9155-charger.h - Header for DA9155
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DA9155_CHARGER_H__
#define __DA9155_CHARGER_H__

#include "../sec_charging_common.h"

#define DA9155_REGS_BASE                    0
#define DA9155_REGS_SIZE                    1024

/*---------------------------------------- */
/* Registers */
/*---------------------------------------- */
#define DA9155_PAGE_CTRL_0                   0x0
#define DA9155_STATUS_A                      0x1
#define DA9155_STATUS_B                      0x2
#define DA9155_EVENT_A                       0x3
#define DA9155_EVENT_B                       0x4
#define DA9155_MASK_A                        0x5
#define DA9155_MASK_B                        0x6
#define DA9155_CONTROL_A                     0x7
#define DA9155_CONTROL_B                     0x8
#define DA9155_CONTROL_C                     0x9
#define DA9155_CONTROL_D                     0xA
#define DA9155_CONTROL_E                     0xB
#define DA9155_TIMER_A                       0xC
#define DA9155_TIMER_B                       0xD
#define DA9155_BUCK_CONT                     0xE
#define DA9155_BUCK_ILIM                     0xF
#define DA9155_BUCK_IOUT                     0x10
#define DA9155_INTERFACE                     0x11
#define DA9155_CONFIG_A                      0x12
#define DA9155_CONFIG_B                      0x13

/*---------------------------------------- */
/* Bit Fields */
/*---------------------------------------- */

/* DA9155_PAGE_CTRL_0     = 0x0 */
#define DA9155_REG_PAGE_SHIFT                0
#define DA9155_REG_PAGE_MASK                 0x7
#define DA9155_WRITE_MODE_SHIFT              6
#define DA9155_WRITE_MODE_MASK               BIT(6)
#define DA9155_REVERT_SHIFT                  7
#define DA9155_REVERT_MASK                   BIT(7)

/* DA9155_STATUS_A     = 0x1 */
#define DA9155_S_TJUNC_WARN_SHIFT            0
#define DA9155_S_TJUNC_WARN_MASK             0x1
#define DA9155_S_TJUNC_CRIT_SHIFT            1
#define DA9155_S_TJUNC_CRIT_MASK             BIT(1)
#define DA9155_S_VBAT_UV_SHIFT               2
#define DA9155_S_VBAT_UV_MASK                BIT(2)
#define DA9155_S_VBAT_OV_SHIFT               3
#define DA9155_S_VBAT_OV_MASK                BIT(3)
#define DA9155_S_VIN_UV_SHIFT                4
#define DA9155_S_VIN_UV_MASK                 BIT(4)
#define DA9155_S_VIN_DROP_SHIFT              5
#define DA9155_S_VIN_DROP_MASK               BIT(5)
#define DA9155_S_VIN_OV_SHIFT                6
#define DA9155_S_VIN_OV_MASK                 BIT(6)
#define DA9155_S_EN_BLOCK_SHIFT              7
#define DA9155_S_EN_BLOCK_MASK               BIT(7)
#define DA9155_S_FAULT_COND_SHIFT            0
#define DA9155_S_FAULT_COND_MASK             0xFE

/* DA9155_STATUS_B     = 0x2 */
#define DA9155_MODE_SHIFT                    0
#define DA9155_MODE_MASK                     0x1
#define DA9155_S_EN_PIN_SHIFT                1
#define DA9155_S_EN_PIN_MASK                 BIT(1)

/* DA9155_EVENT_A     = 0x3 */
#define DA9155_E_TJUNC_WARN_SHIFT            0
#define DA9155_E_TJUNC_WARN_MASK             0x1
#define DA9155_E_TJUNC_CRIT_SHIFT            1
#define DA9155_E_TJUNC_CRIT_MASK             BIT(1)
#define DA9155_E_VBAT_UV_SHIFT               2
#define DA9155_E_VBAT_UV_MASK                BIT(2)
#define DA9155_E_VBAT_OV_SHIFT               3
#define DA9155_E_VBAT_OV_MASK                BIT(3)
#define DA9155_E_VIN_UV_SHIFT                4
#define DA9155_E_VIN_UV_MASK                 BIT(4)
#define DA9155_E_VIN_DROP_SHIFT              5
#define DA9155_E_VIN_DROP_MASK               BIT(5)
#define DA9155_E_VIN_OV_SHIFT                6
#define DA9155_E_VIN_OV_MASK                 BIT(6)
#define DA9155_E_EN_BLOCK_SHIFT              7
#define DA9155_E_EN_BLOCK_MASK               BIT(7)
#define DA9155_E_FAULT_SHIFT                 0
#define DA9155_E_FAULT_MASK                  0x7E

/* DA9155_EVENT_B     = 0x4 */
#define DA9155_E_RDY_SHIFT                   0
#define DA9155_E_RDY_MASK                    0x1
#define DA9155_E_BUCK_ILIM_SHIFT             1
#define DA9155_E_BUCK_ILIM_MASK              BIT(1)
#define DA9155_E_TIMER_SHIFT                 2
#define DA9155_E_TIMER_MASK                  BIT(2)
#define DA9155_E_VDDIO_UV_SHIFT              3
#define DA9155_E_VDDIO_UV_MASK               BIT(3)
#define DA9155_E_TJUNC_POR_SHIFT             4
#define DA9155_E_TJUNC_POR_MASK              BIT(4)

/* DA9155_MASK_A     = 0x5 */
#define DA9155_M_TJUNC_WARN_SHIFT            0
#define DA9155_M_TJUNC_WARN_MASK             0x1
#define DA9155_M_TJUNC_CRIT_SHIFT            1
#define DA9155_M_TJUNC_CRIT_MASK             BIT(1)
#define DA9155_M_VBAT_UV_SHIFT               2
#define DA9155_M_VBAT_UV_MASK                BIT(2)
#define DA9155_M_VBAT_OV_SHIFT               3
#define DA9155_M_VBAT_OV_MASK                BIT(3)
#define DA9155_M_VIN_UV_SHIFT                4
#define DA9155_M_VIN_UV_MASK                 BIT(4)
#define DA9155_M_VIN_DROP_SHIFT              5
#define DA9155_M_VIN_DROP_MASK               BIT(5)
#define DA9155_M_VIN_OV_SHIFT                6
#define DA9155_M_VIN_OV_MASK                 BIT(6)
#define DA9155_M_EN_BLOCK_SHIFT              7
#define DA9155_M_EN_BLOCK_MASK               BIT(7)

/* DA9155_MASK_B     = 0x6 */
#define DA9155_M_RDY_SHIFT                   0
#define DA9155_M_RDY_MASK                    0x1
#define DA9155_M_BUCK_ILIM_SHIFT             1
#define DA9155_M_BUCK_ILIM_MASK              BIT(1)
#define DA9155_M_TIMER_SHIFT                 2
#define DA9155_M_TIMER_MASK                  BIT(2)
#define DA9155_M_VDDIO_UV_SHIFT              3
#define DA9155_M_VDDIO_UV_MASK               BIT(3)
#define DA9155_M_TJUNC_POR_SHIFT             4
#define DA9155_M_TJUNC_POR_MASK              BIT(4)

/* DA9155_CONTROL_A     = 0x7 */
#define DA9155_VIN_DROP_SHIFT                0
#define DA9155_VIN_DROP_MASK                 0xFF

/* DA9155_CONTROL_B     = 0x8 */
#define DA9155_VBAT_UV_SHIFT                 0
#define DA9155_VBAT_UV_MASK                  0x3F

/* DA9155_CONTROL_C     = 0x9 */
#define DA9155_VBAT_OV_SHIFT                 0
#define DA9155_VBAT_OV_MASK                  0x3F

/* DA9155_CONTROL_D     = 0xa */
#define DA9155_DEF_SLEW_RATE_SHIFT           0
#define DA9155_DEF_SLEW_RATE_MASK            0x7
#define DA9155_START_SLEW_SHIFT              4
#define DA9155_START_SLEW_MASK               (0x7 << 4)

/* DA9155_CONTROL_E     = 0xb */
#define DA9155_TIMER_DIS_SHIFT               4
#define DA9155_TIMER_DIS_MASK                0x10
#define DA9155_TJUNC_WARN_SHIFT              0
#define DA9155_TJUNC_WARN_MASK               0xF

/* DA9155_TIMER_A     = 0xc */
#define DA9155_TIMER_COUNT_SHIFT             0
#define DA9155_TIMER_COUNT_MASK              0xFF

/* DA9155_TIMER_B     = 0xd */
#define DA9155_TIMER_LOAD_SHIFT              0
#define DA9155_TIMER_LOAD_MASK               0xFF
#define DA9155_TIMER_LOAD_DEFAULT            0xFF

/* DA9155_BUCK_CONT     = 0xe */
#define DA9155_BUCK_EN_SHIFT                 0
#define DA9155_BUCK_EN_MASK                  0x1

/* DA9155_BUCK_ILIM     = 0xf */
#define DA9155_BUCK_ILIM_SHIFT               0
#define DA9155_BUCK_ILIM_MASK                0x1F

/* DA9155_BUCK_IOUT     = 0x10 */
#define DA9155_BUCK_IOUT_SHIFT               0
#define DA9155_BUCK_IOUT_MASK                0xFF

/* DA9155_INTERFACE     = 0x11 */
#define DA9155_IF_BASE_ADDR_SHIFT            1
#define DA9155_IF_BASE_ADDR_MASK             (0x7F << 1)

/* DA9155_CONFIG_A     = 0x12 */
#define DA9155_IRQ_TYPE_SHIFT                0
#define DA9155_IRQ_TYPE_MASK                 0x1
#define DA9155_IRQ_LEVEL_SHIFT               1
#define DA9155_IRQ_LEVEL_MASK                BIT(1)
#define DA9155_PM_IF_HSM_SHIFT               3
#define DA9155_PM_IF_HSM_MASK                BIT(3)
#define DA9155_I2C_TO_EN_SHIFT               4
#define DA9155_I2C_TO_EN_MASK                BIT(4)
#define DA9155_I2C_EXTEND_EN_SHIFT           5
#define DA9155_I2C_EXTEND_EN_MASK            BIT(5)
#define DA9155_VDDIO_CONF_SHIFT              6
#define DA9155_VDDIO_CONF_MASK               (0x3 << 6)

/* DA9155_CONFIG_B     = 0x13 */
#define DA9155_BUCK_FSW_SHIFT                0
#define DA9155_BUCK_FSW_MASK                 0x3
#define DA9155_OSC_FRQ_SHIFT                 4
#define DA9155_OSC_FRQ_MASK                  (0xF << 4)

struct da9155_charger_platform_data {
	int irq_gpio;
};

struct da9155_charger_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct mutex            io_lock;

	struct da9155_charger_platform_data *pdata;

	struct power_supply	*psy_chg;

	unsigned int siop_level;
	unsigned int chg_irq;
	unsigned int is_charging;
	unsigned int charging_type;
	unsigned int cable_type;
	unsigned int prev_cable_type;
	unsigned int charging_current_max;
	unsigned int charging_current;
	unsigned int float_voltage;

	u8 addr;
	int size;
};


#endif	/* __DA9155_CHARGER_H__ */
