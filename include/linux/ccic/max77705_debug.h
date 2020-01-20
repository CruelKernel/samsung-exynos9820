/*
 * Copyright (c) 2017 Maxim Integrated Products, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define OPCODE_READ_MAX_LENGTH		(32 + 1)
#define OPCODE_WRITE_MAX_LENGTH     (32 + 1) /* opcode + data 31bytes */
#define OPCODE_WRITE_START_ADDR		0x21
#define OPCODE_WRITE_END_ADDR		0x41
#define OPCODE_READ_START_ADDR		0x51
#define OPCODE_READ_END_ADDR		0x71

enum mxim_ioctl_cmd {
	MXIM_DEBUG_OPCODE_WRITE = 1000,
	MXIM_DEBUG_OPCODE_READ,
	MXIM_DEBUG_REG_WRITE = 1002,
	MXIM_DEBUG_REG_READ,
	MXIM_DEBUG_REG_DUMP,
	MXIM_DEBUG_FW_VERSION = 1005,
};

enum mxim_registers {
	MXIM_REG_UIC_HW_REV = 0x00,
	MXIM_REG_UIC_FW_REV,
	MXIM_REG_USBC_IRQ = 0x02,
	MXIM_REG_CC_IRQ,
	MXIM_REG_PD_IRQ,
	MXIM_REG_RSVD1,
	MXIM_REG_USBC_STATUS1 = 0x06,
	MXIM_REG_USBC_STATUS2,
	MXIM_REG_BC_STATUS,
	MXIM_REG_RSVD2,
	MXIM_REG_CC_STATUS1 = 0x0A,
	MXIM_REG_CC_STATUS2,
	MXIM_REG_PD_STATUS1,
	MXIM_REG_PD_STATUS2,
	MXIM_REG_USBC_IRQM = 0x0E,
	MXIM_REG_CC_IRQM,
	MXIM_REG_PD_IRQM,
	MXIM_REG_MAX,
};

struct mxim_debug_registers {
	unsigned char reg;
	unsigned char val;
	int ignore;
};

struct mxim_debug_pdev {
	struct mxim_debug_registers registers;
	struct i2c_client *client;
	struct class *class;
	struct device *dev;
	struct mutex lock;
	unsigned char opcode_wdata[OPCODE_WRITE_MAX_LENGTH];
	unsigned char opcode_rdata[OPCODE_READ_MAX_LENGTH];
};

void mxim_debug_set_i2c_client(struct i2c_client *client);
int mxim_debug_init(void);
void mxim_debug_exit(void);
