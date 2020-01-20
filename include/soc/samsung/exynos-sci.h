/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_SCI_H_
#define __EXYNOS_SCI_H_

#define EXYNOS_SCI_MODULE_NAME		"exynos-sci"
#define LLC_DSS_NAME			"log_llc"

#define LLC_DUMP_DATA_Q_SIZE			8
#define LLC_SLICE_END				(0x1)
#define LLC_BANK_END				(0x1)
#define LLC_SET_END				(0x1FF)
#define LLC_WAY_END				(0xF)
#define LLC_QWORD_END				(0x7)

#define SCI_BASE				0x1A000000
#define ArrDbgCntl				0x05C
#define ArrDbgRDataHi				0x06C
#define ArrDbgRDataMi				0x070
#define ArrDbgRDataLo				0x074
#define CCMControl1				0x0A8
#define PM_SCI_CTL				0x140

#define LLC_En_Bit				(25)
#define DisableLlc_Bit				(9)

/* IPC common definition */
#define SCI_ONE_BIT_MASK			(0x1)
#define SCI_ERR_MASK				(0x7)
#define SCI_ERR_SHIFT				(13)

#define SCI_CMD_IDX_MASK			(0x3F)
#define SCI_CMD_IDX_SHIFT			(0)
#define SCI_DATA_MASK				(0x3F)
#define SCI_DATA_SHIFT				(6)
#define SCI_IPC_DIR_SHIFT			(12)

#define SCI_CMD_GET(cmd_data, mask, shift)	((cmd_data & (mask << shift)) >> shift)
#define SCI_CMD_CLEAR(mask, shift)		(~(mask << shift))
#define SCI_CMD_SET(data, mask, shift)		((data & mask) << shift)

#define SCI_DBGGEN
#ifdef SCI_DBGGEN
#define SCI_DBG(x...)	pr_info("sci_dbg: " x)
#else
#define SCI_DBG(x...)	do {} while (0)
#endif

#define SCI_INFO(x...)	pr_info("sci_info: " x)
#define SCI_ERR(x...)	pr_err("sci_err: " x)

enum exynos_sci_cmd_index {
	SCI_LLC_INVAL = 0,
	SCI_LLC_FLUSH,
	SCI_LLC_REGION_ALLOC,
	SCI_LLC_EN,
	SCI_CMD_MAX,
};

enum exynos_sci_err_code {
	E_OK_S = 0,
	E_INVAL_S,
	E_BUSY_S,
};

enum exynos_sci_ipc_dir {
	SCI_IPC_GET = 0,
	SCI_IPC_SET,
};

enum exynos_sci_llc_region_index {
	LLC_REGION_DISABLE = 0,
	LLC_REGION_LIT_MID_ALL,
	LLC_REGION_CPU_ALL,
	LLC_REGION_CPU_2_GPU_2,
	LLC_REGION_BIG_ALL,
	LLC_REGION_GPU_ALL,
	LLC_REGION_LIT_MID_GPU_SHARE,
	LLC_REGION_CPU_GPU_SHARE,
	LLC_REGION_BIG_GPU_SHARE,
	LLC_REGION_MAX,
};

struct exynos_sci_cmd_info {
	enum exynos_sci_cmd_index	cmd_index;
	enum exynos_sci_ipc_dir		direction;
	unsigned int			data;
};

struct exynos_llc_dump_data {
	u32				data_lo;
	u32				data_mi;
	u32				data_hi;
};

struct exynos_llc_dump_fmt {
	u16				slice;
	u16				bank;
	u16				set;
	u16				way;
	struct exynos_llc_dump_data	tag;
	struct exynos_llc_dump_data	data_q[LLC_DUMP_DATA_Q_SIZE];
} __attribute__((packed));

struct exynos_llc_dump_addr {
        u32                             p_addr;
        u32                             p_size;
        u32                             buff_size;
        void __iomem                    *v_addr;
};

struct exynos_sci_data {
	struct device			*dev;
	spinlock_t			lock;

	unsigned int			ipc_ch_num;
	unsigned int			ipc_ch_size;

	unsigned int			use_init_llc_region;
	unsigned int			initial_llc_region;
	unsigned int			llc_enable;

	unsigned int 			plugin_init_llc_region;
	unsigned int			llc_region_prio[LLC_REGION_MAX];

	void __iomem			*sci_base;
	struct exynos_llc_dump_addr	llc_dump_addr;
};

#ifdef CONFIG_EXYNOS_SCI
void llc_invalidate(void);
void llc_flush(void);
void llc_region_alloc(unsigned int region_index, bool on);
void llc_dump(void);
void sci_error_dump(void);
#else
#define llc_invalidate() do {} while (0)
#define llc_flush() do {} while (0)
#define llc_region_alloc(a, b) do {} while (0)
#define llc_dump() do {} while (0)
#define sci_error_dump() do {} while (0)
#endif

#endif	/* __EXYNOS_SCI_H_ */
