/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Per-Page Memory Protection Unit(PPMPU) fail detector
 * Author: Junho Choi <junhosj.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_PPMPU_H
#define __EXYNOS_PPMPU_H

/* The maximum number of PPMPU channel */
#define MAX_NUM_OF_PPMPU_CHANNEL			(4)

/* SFR bits information */
/* PPMPU Interrupt Status Register */
#define PPMPU_WRITE_INTR_STATUS_OVERRUN			(1 << 3)
#define PPMPU_READ_INTR_STATUS_OVERRUN			(1 << 2)
#define PPMPU_WRITE_INTR_STATUS				(1 << 1)
#define PPMPU_READ_INTR_STATUS				(1 << 0)

/* PPMPU Illegal Read/Write Address High Register */
#define PPMPU_ILLEGAL_ADDR_HIGH_MASK			(0xFF << 0)

/* PPMPU Illegal Read/Write Field Register */
#define PPMPU_ILLEGAL_FIELD_FAIL_VC_MASK		(0xF << 24)
#define PPMPU_ILLEGAL_FIELD_FAIL_ID_MASK		(0x3FFFF << 0)

/* Return values from SMC */
#define PPMPU_ERROR_INVALID_CH_NUM			(0xA00A)
#define PPMPU_ERROR_INVALID_FAIL_INFO_SIZE		(0xA00B)

#define PPMPU_NEED_FAIL_INFO_LOGGING			(0x1EED)
#define PPMPU_SKIP_FAIL_INFO_LOGGING			(0x2419)

/* Flag whether fail read information is logged */
#define STR_INFO_FLAG					(0x50504D50)	/* PPMP */

/* Direction of illegal access */
#define PPMPU_ILLEGAL_ACCESS_READ			(0)
#define PPMPU_ILLEGAL_ACCESS_WRITE			(1)

#define PPMPU_DIRECTION_SHIFT				(16)

#ifndef __ASSEMBLY__
/* Registers of PPMPU Fail Information */
struct ppmpu_fail_info {
	unsigned int ppmpu_intr_stat;
	unsigned int ppmpu_illegal_read_addr_low;
	unsigned int ppmpu_illegal_read_addr_high;
	unsigned int ppmpu_illigal_read_field;
	unsigned int ppmpu_illegal_write_addr_low;
	unsigned int ppmpu_illegal_write_addr_high;
	unsigned int ppmpu_illigal_write_field;
};

/* Data structure for PPMPU Fail Information */
struct ppmpu_info_data {
	struct device *dev;

	struct ppmpu_fail_info *fail_info;
	dma_addr_t fail_info_pa;
	unsigned int ch_num;

	unsigned int irq[MAX_NUM_OF_PPMPU_CHANNEL * 2];
	unsigned int irqcnt;

	unsigned int info_flag;
	int need_log;
};
#endif	/* __ASSEMBLY__ */
#endif	/* __EXYNOS_PPMPU_H */
