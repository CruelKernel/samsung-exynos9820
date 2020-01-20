/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_DW9780_H
#define FIMC_IS_DEVICE_DW9780_H

#define DW9780_POS_SIZE_BIT		ACTUATOR_POS_SIZE_10BIT
#define DW9780_POS_MAX_SIZE		((1 << DW9780_POS_SIZE_BIT) - 1)
#define DW9780_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC

/* === Chip info === */
#define DW9780_REG_CHIP_ID		(0x5700)
#define DW9780_REG_FW_CHKSUM		(0x5701)
#define DW9780_REG_FW_VER		(0x5703)

/* === Mode control === */
/* CMD_UPD
 * bit[15]: Command execution - 0: Disable / 1: Enable
 * bit[12]: Operation start - 0: Disable / 1: Enable
 */
#define DW9780_REG_CMD_UPD		(0x5705)
/* OP_MODE
 * bit[15:8]: Mode selection - 0x80: OIS mode / 0x40: Cal. mode
 * bit[7:0]: Function select
	- 0x10: Diagnosis mode
	- 0x11: Hall amp calibration
	- 0x12: Hall offset calibration
	- 0x13: Servo loop gain calibration
	- 0x14: Manual mode
	- 0x15: Gyro offset
	- 0x16: FRA
 */
#define DW9780_REG_OP_MODE		(0x5708)

/* === OIS control === */
/* OIS_CON
 * bit[1:0]: OIS control - 0: OIS on & Servo on / 1: OIS off & Servo on / 2: OIS off & Servo off
 */
#define DW9780_REG_OIS_CON		(0x57A9)

/* === Operation control === */
#define DW9780_REG_CHIP_ACT		(0x8000)
#define DW9780_REG_OLAF_TARGET		(0x8001)
#define DW9780_REG_SAVE_LOAD		(0x8004)
/* OLAF_ACT
 * bit[4]: Open loop AF function enable - 0: Disable / 1: Enable
 * bit[3]: Ringing control mode - 0: Disable / 1: Enable
 * bit[2]: SAC+ setup - 0: Disable / 1: Enable
 * bit[1:0]: SAC setup - 00b: SAC2.0 / 01b: SAC 2.5 / 10b: SAC 3.0 / 11b: SAC 3.5
 */
#define DW9780_REG_OLAF_ACT		(0x8006)
/* OLAF_CONF
 * bit[10:8]: Prescaler selection - 00b: Tvib x1 / 01b: x2 / 10b: x4 / 11b: x1
 * bit[6:0]: 7bit SAC time control - counter number selection
 */
#define DW9780_REG_OLAF_CONF		(0x8015)
#define DW9780_REG_USER_CONTROL_PROTECTION	(0xEBF1)

struct fimc_is_caldata_list_dw9780 {
	u32 af_position_type;
	u32 af_position_worst;
	u32 af_macro_position_type;
	u32 af_macro_position_worst;
	u32 af_default_position;
	u8 reserved0[12];

	u16 info_position;
	u16 mac_position;
	u32 equipment_info1;
	u32 equipment_info2;
	u32 equipment_info3;
	u8 control_mode;
	u8 prescale;
	u8 resonance;
	u8 reserved1[13];
	u8 reserved2[16];

	u8 core_version;
	u8 pixel_number[2];
	u8 is_pid;
	u8 sensor_maker;
	u8 year;
	u8 month;
	u16 release_number;
	u8 manufacturer_id;
	u8 module_version;
	u8 reserved3[161];
	u32 check_sum;
};

#endif
