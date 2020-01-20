/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Logging message from Secure World
 * Author: Junho Choi <junhosj.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_SECLOG_H
#define __EXYNOS_SECLOG_H

/* Error code */
#define ERROR_INVALID_LOG_LEN			(-1)
#define ERROR_INVALID_LOG_ADDR			(-2)
#define ERROR_INVALID_INTR_NUM			(-3)
#define ERROR_ALREADY_INITIALIZED		(-4)

/* Error code from LDFW */
#define ERROR_LDFW_ALREADY_INITIALIZED		(1)
#define ERROR_LDFW_CALL_FROM_NON_SECURE		(2)
#define ERROR_NOT_SUPPORT_LDFW_ERR_VALUE	(0xE)
#define ERROR_NOT_SUPPORT_LDFW_SEC_LOG		(0xF)

#define BITLEN_LDFW_ERROR			(4)
#define MASK_LDFW_ERROR				(0xF)

#define SHIFT_LDFW_MAGIC			(28)
#define MASK_LDFW_MAGIC				(0xF << SHIFT_LDFW_MAGIC)
#define LDFW_MAGIC				(0xA << SHIFT_LDFW_MAGIC)

#define LDFW_MAX_NUM				(7)

/* Secure log buffer information */
#define SECLOG_LOG_BUF_SIZE			(0x10000)

/* Alignment with 4 bytes */
#define FOUR_BYTES_SHIFT			(2)
#define FOUR_BYTES_MASK				((1 << FOUR_BYTES_SHIFT) - 1)

/*
 * If input address is not aligned with 4 bytes,
 * it makes this address be aligned with next 4 bytes.
 * Otherwise, there is no action.
 */
#define CHECK_AND_ALIGN_4BYTES(addr)		do {		\
		if ((addr) & FOUR_BYTES_MASK) {			\
			addr &= ~FOUR_BYTES_MASK;		\
			addr += (1 << FOUR_BYTES_SHIFT);	\
		}						\
	} while (0)

#ifndef __ASSEMBLY__
/* Reserved memory data */
struct seclog_data {
	void *virt_addr;
	unsigned long phys_addr;
	unsigned long size;
};

/* Context for Secure log */
struct seclog_ctx {
	struct work_struct work;
	/* debugfs root */
	struct dentry *debug_dir;
	/* seclog can be disabled via debugfs */
	bool enabled;
	unsigned int irq;
};

/* Log header information */
struct log_header_info {
	unsigned int log_len;
	unsigned int tv_sec;
	unsigned int tv_usec;
};

/* Secure log information shared with EL3 Monitor and LDFWs */
struct sec_log_info {
	/* The count to write log */
	unsigned int log_write_cnt;
	/* The count to read log */
	unsigned int log_read_cnt;
	/*
	 * The log count when log_buf
	 * returns to initial_log_buf
	 */
	unsigned int log_return_cnt;
	/* Start log buffer address */
	unsigned long start_log_addr;
	/* Initial log buffer address */
	unsigned long initial_log_addr;
};
#endif	/* __ASSEMBLY__ */

#endif	/* __EXYNOS_SECLOG_H */
