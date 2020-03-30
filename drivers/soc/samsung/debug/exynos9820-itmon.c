/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * IPs Traffic Monitor(ITMON) Driver for Samsung Exynos9820 SOC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/pm_domain.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-itmon.h>
#include <soc/samsung/exynos-debug.h>
#include <soc/samsung/exynos-pd.h>
#include <linux/debug-snapshot.h>
#if defined(CONFIG_SEC_SIPC_MODEM_IF)
#include <soc/samsung/exynos-modem-ctrl.h>
#endif
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif

//#define MULTI_IRQ_SUPPORT_ITMON

#define OFFSET_TMOUT_REG		(0x2000)
#define OFFSET_REQ_R			(0x0)
#define OFFSET_REQ_W			(0x20)
#define OFFSET_RESP_R			(0x40)
#define OFFSET_RESP_W			(0x60)
#define OFFSET_ERR_REPT			(0x20)
#define OFFSET_HW_ASSERT		(0x100)
#define OFFSET_NUM			(0x4)

#define REG_INT_MASK			(0x0)
#define REG_INT_CLR			(0x4)
#define REG_INT_INFO			(0x8)
#define REG_EXT_INFO_0			(0x10)
#define REG_EXT_INFO_1			(0x14)
#define REG_EXT_INFO_2			(0x18)

#define REG_DBG_CTL			(0x10)
#define REG_TMOUT_INIT_VAL		(0x14)
#define REG_TMOUT_FRZ_EN		(0x18)
#define REG_TMOUT_BUF_WR_OFFSET		(0x20)

#define REG_TMOUT_BUF_STATUS		(0x1C)
#define REG_TMOUT_BUF_POINT_ADDR	(0x20)
#define REG_TMOUT_BUF_ID		(0x24)
#define REG_TMOUT_BUF_PAYLOAD		(0x28)
#define REG_TMOUT_BUF_PAYLOAD_SRAM1	(0x30)
#define REG_TMOUT_BUF_PAYLOAD_SRAM2	(0x34)
#define REG_TMOUT_BUF_PAYLOAD_SRAM3	(0x38)

#define REG_HWA_CTL			(0x4)
#define REG_HWA_INT			(0x8)
#define REG_HWA_INT_ID			(0xC)
#define REG_HWA_START_ADDR_LOW		(0x10)
#define REG_HWA_END_ADDR_LOW		(0x14)
#define REG_HWA_START_END_ADDR_UPPER	(0x18)

#define RD_RESP_INT_ENABLE		(1 << 0)
#define WR_RESP_INT_ENABLE		(1 << 1)
#define ARLEN_RLAST_INT_ENABLE		(1 << 2)
#define AWLEN_WLAST_INT_ENABLE		(1 << 3)
#define INTEND_ACCESS_INT_ENABLE	(1 << 4)

#define BIT_HWA_ERR_OCCURRED(x)		(((x) & (0x1 << 0)) >> 0)
#define BIT_HWA_ERR_CODE(x)		(((x) & (0xF << 1)) >> 28)

#define BIT_ERR_CODE(x)			(((x) & (0xF << 28)) >> 28)
#define BIT_ERR_OCCURRED(x)		(((x) & (0x1 << 27)) >> 27)
#define BIT_ERR_VALID(x)		(((x) & (0x1 << 26)) >> 26)
#define BIT_AXID(x)			(((x) & (0xFFFF)))
#define BIT_AXUSER(x)			(((x) & (0xFFFF << 16)) >> 16)
#define BIT_AXBURST(x)			(((x) & (0x3)))
#define BIT_AXPROT(x)			(((x) & (0x3 << 2)) >> 2)
#define BIT_AXLEN(x)			(((x) & (0xF << 16)) >> 16)
#define BIT_AXSIZE(x)			(((x) & (0x7 << 28)) >> 28)

#define M_NODE				(0)
#define T_S_NODE			(1)
#define T_M_NODE			(2)
#define S_NODE				(3)
#define NODE_TYPE			(4)

#define ERRCODE_SLVERR			(0)
#define ERRCODE_DECERR			(1)
#define ERRCODE_UNSUPORTED		(2)
#define ERRCODE_POWER_DOWN		(3)
#define ERRCODE_UNKNOWN_4		(4)
#define ERRCODE_UNKNOWN_5		(5)
#define ERRCODE_TMOUT			(6)

#define BUS_DATA			(0)
#define BUS_PERI			(1)
#define BUS_PATH_TYPE			(2)

#define TRANS_TYPE_WRITE		(0)
#define TRANS_TYPE_READ			(1)
#define TRANS_TYPE_NUM			(2)

#define FROM_CP				(0)
#define FROM_CPU			(2)
#define FROM_M_NODE			(4)

#define CP_COMMON_STR			"CP_"
#define DREX_COMMON_STR			"DREX_IRPS"

#define TMOUT				(0xFFFFF)
#define TMOUT_TEST			(0x1)

#define PANIC_ALLOWED_THRESHOLD		(0x2)
#define INVALID_REMAPPING		(0x03000000)

/* This value will be fixed */
#define INTEND_ADDR_START		(0)
#define INTEND_ADDR_END			(0)

struct itmon_rpathinfo {
	unsigned int id;
	char *port_name;
	char *dest_name;
	unsigned int bits;
	unsigned int shift_bits;
};

struct itmon_masterinfo {
	char *port_name;
	unsigned int user;
	char *master_name;
	unsigned int bits;
};

struct itmon_nodegroup;

struct itmon_traceinfo {
	char *port;
	char *master;
	char *dest;
	unsigned long target_addr;
	unsigned int errcode;
	bool read;
	bool onoff;
	char *pd_name;
	bool path_dirty;
	bool snode_dirty;
	bool dirty;
	unsigned long from;
	int path_type;
	char buf[SZ_32];
};

struct itmon_tracedata {
	unsigned int int_info;
	unsigned int ext_info_0;
	unsigned int ext_info_1;
	unsigned int ext_info_2;
	unsigned int dbg_mo_cnt;
	unsigned int hwa_ctl;
	unsigned int hwa_info;
	unsigned int hwa_int_id;
	unsigned int offset;
	bool logging;
	bool read;
};

struct itmon_nodeinfo {
	unsigned int type;
	char *name;
	unsigned int phy_regs;
	void __iomem *regs;
	unsigned int time_val;
	bool tmout_enabled;
	bool tmout_frz_enabled;
	bool err_enabled;
	bool hw_assert_enabled;
	bool addr_detect_enabled;
	char *pd_name;
	bool retention;
	struct itmon_tracedata tracedata;
	struct itmon_nodegroup *group;
	struct list_head list;
};

struct itmon_nodegroup {
	int irq;
	char *name;
	unsigned int phy_regs;
	bool ex_table;
	void __iomem *regs;
	struct itmon_nodeinfo *nodeinfo;
	unsigned int nodesize;
	unsigned int bus_type;
};

struct itmon_platdata {
	const struct itmon_rpathinfo *rpathinfo;
	const struct itmon_masterinfo *masterinfo;
	struct itmon_nodegroup *nodegroup;
	struct itmon_traceinfo traceinfo[TRANS_TYPE_NUM];
	struct list_head tracelist[TRANS_TYPE_NUM];
	unsigned int err_cnt;
	unsigned int err_cnt_by_cpu;
	ktime_t last_time;
	bool panic_allowed;
	bool crash_in_progress;
	unsigned int sysfs_tmout_val;
	bool sysfs_scandump;
	bool sysfs_s2d;
	bool err_fatal;
	bool err_hwa;
	bool probed;
};

struct itmon_dev {
	struct device *dev;
	struct itmon_platdata *pdata;
	struct of_device_id *match;
	int irq;
	int id;
	void __iomem *regs;
	spinlock_t ctrl_lock;
	struct itmon_notifier notifier_info;
};

struct itmon_panic_block {
	struct notifier_block nb_panic_block;
	struct itmon_dev *pdev;
};

const static struct itmon_rpathinfo rpathinfo[] = {
	/*
	 * 0x2000_0000 ~ 0xf_ffff_ffff
	 */
	{0,	"DPU0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{1,	"DPU1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{2,	"DPU2",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{3,	"ISPPRE",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{4,	"ISPLP0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{5,	"ISPLP1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{6,	"ISPHQ",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{7,	"NPU",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{8,	"VRA2",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{9,	"IVA",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{10,	"DSPM0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{11,	"DSPM1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{12,	"MFC0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{13,	"MFC1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{14,	"G2D0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{15,	"G2D1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{16,	"G2D1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{17,	"FSYS0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{18,	"FSYS1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{19,	"AVPS",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{20,	"SBIC",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{21,	"DIT",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{22,	"SIREX",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{23,	"AUD",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{24,	"M-CACHE",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{25,	"DPU0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{26,	"DPU1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{27,	"DPU2",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{28,	"ISPPRE",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{29,	"ISPLP0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{30,	"ISPLP1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{31,	"ISPHQ",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{32,	"NPU",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{33,	"VRA2",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{34,	"IVA",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{35,	"DSPM0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{36,	"DSPM1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{37,	"MFC0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{38,	"MFC1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{39,	"G2D0",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{40,	"G2D1",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{41,	"G2D2",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{42,	"FSYS0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{43,	"FSYS1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{44,	"AVPS",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{45,	"SBIC",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{46,	"DIT",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{47,	"SIREX",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{48,	"AUD",		"DREX_IRPS",	GENMASK(5, 0),	0},
	{49,	"M-CACHE",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{50,	"CP_DATA_0",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{51,	"CP_DATA_1",	"DREX_IRPS",	GENMASK(5, 0),	0},
	{52,	"CORESIGHT",	"DREX_IRPS",	GENMASK(5, 0),	0},

	/*
	 * 0x0400_0000 ~ 0x040f_ffff
	 */
	{0,	"DPU0",		"SP",		GENMASK(4, 0),	0},
	{1,	"DPU1",		"SP",		GENMASK(4, 0),	0},
	{2,	"DPU2",		"SP",		GENMASK(4, 0),	0},
	{3,	"ISPPRE",	"SP",		GENMASK(4, 0),	0},
	{4,	"ISPLP0",	"SP",		GENMASK(4, 0),	0},
	{5,	"ISPLP1",	"SP",		GENMASK(4, 0),	0},
	{6,	"ISPHQ",	"SP",		GENMASK(4, 0),	0},
	{7,	"NPU",		"SP",		GENMASK(4, 0),	0},
	{8,	"VRA2",		"SP",		GENMASK(4, 0),	0},
	{9,	"IVA",		"SP",		GENMASK(4, 0),	0},
	{10,	"DSPM0",	"SP",		GENMASK(4, 0),	0},
	{11,	"DSPM1",	"SP",		GENMASK(4, 0),	0},
	{12,	"MFC0",		"SP",		GENMASK(4, 0),	0},
	{13,	"MFC1",		"SP",		GENMASK(4, 0),	0},
	{14,	"G2D0",		"SP",		GENMASK(4, 0),	0},
	{15,	"G2D1",		"SP",		GENMASK(4, 0),	0},
	{16,	"G2D2",		"SP",		GENMASK(4, 0),	0},
	{17,	"FSYS0",	"SP",		GENMASK(4, 0),	0},
	{18,	"FSYS1",	"SP",		GENMASK(4, 0),	0},
	{19,	"AVPS",		"SP",		GENMASK(4, 0),	0},
	{20,	"SBIC",		"SP",		GENMASK(4, 0),	0},
	{21,	"DIT",		"SP",		GENMASK(4, 0),	0},
	{22,	"SIREX",	"SP",		GENMASK(4, 0),	0},
	{23,	"AUD",		"SP",		GENMASK(4, 0),	0},
	{24,	"BUSC_PD",	"SP",		GENMASK(4, 0),	0},

	/*
	 * 0x0000_0000 ~ 0x03ff_ffff
	 * 0x0410_0000 ~ 0x7fff_ffff
	 */
	{0,	"DPU0",		"BUSC_DP",	GENMASK(4, 0),	0},
	{1,	"DPU1",		"BUSC_DP",	GENMASK(4, 0),	0},
	{2,	"DPU2",		"BUSC_DP",	GENMASK(4, 0),	0},
	{3,	"ISPPRE",	"BUSC_DP",	GENMASK(4, 0),	0},
	{4,	"ISPLP0",	"BUSC_DP",	GENMASK(4, 0),	0},
	{5,	"ISPLP1",	"BUSC_DP",	GENMASK(4, 0),	0},
	{6,	"ISPHQ",	"BUSC_DP",	GENMASK(4, 0),	0},
	{7,	"NPU",		"BUSC_DP",	GENMASK(4, 0),	0},
	{8,	"VRA2",		"BUSC_DP",	GENMASK(4, 0),	0},
	{9,	"IVA",		"BUSC_DP",	GENMASK(4, 0),	0},
	{10,	"DSPM0",	"BUSC_DP",	GENMASK(4, 0),	0},
	{11,	"DSPM1",	"BUSC_DP",	GENMASK(4, 0),	0},
	{12,	"MFC0",		"BUSC_DP",	GENMASK(4, 0),	0},
	{13,	"MFC1",		"BUSC_DP",	GENMASK(4, 0),	0},
	{14,	"G2D0",		"BUSC_DP",	GENMASK(4, 0),	0},
	{15,	"G2D1",		"BUSC_DP",	GENMASK(4, 0),	0},
	{16,	"G2D2",		"BUSC_DP",	GENMASK(4, 0),	0},
	{17,	"FSYS0",	"BUSC_DP",	GENMASK(4, 0),	0},
	{18,	"FSYS1",	"BUSC_DP",	GENMASK(4, 0),	0},
	{19,	"AVPS",		"BUSC_DP",	GENMASK(4, 0),	0},
	{20,	"SBIC",		"BUSC_DP",	GENMASK(4, 0),	0},
	{21,	"DIT",		"BUSC_DP",	GENMASK(4, 0),	0},
	{22,	"SIREX",	"BUSC_DP",	GENMASK(4, 0),	0},
	{23,	"AUD",		"BUSC_DP",	GENMASK(4, 0),	0},
	/*
	 * 0x0000_0000 ~ 0x1fff_ffff
	 */
	{0,	"CP_0",		"CORE_DP",	GENMASK(1, 0),	0},
	{1,	"CP_1",		"CORE_DP",	GENMASK(1, 0),	0},
	{2,	"CORESIGHT",	"CORE_DP",	GENMASK(1, 0),	0},
};

const static struct itmon_masterinfo masterinfo[] = {
	{"DPU0", 0,			/* 0XXXX0 */ "DPP",		BIT(5) | BIT(0)},
	{"DPU0", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU0",	BIT(5) | BIT(0)},

	{"DPU1", 0,			/* 0XXXX0 */ "DPP",		BIT(5) | BIT(0)},
	{"DPU1", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU1",	BIT(5) | BIT(0)},

	{"DPU2", 0,			/* 0XXXX0 */ "DPP",		BIT(5) | BIT(0)},
	{"DPU2", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU2",	BIT(5) | BIT(0)},

	{"ISPPRE", 0,			/* 0X0000 */ "CSISX4_PDP",	BIT(5) | GENMASK(3, 0)},
	{"ISPPRE", BIT(1),		/* 0X0010 */ "PDP_TOP",		BIT(5) | GENMASK(3, 0)},
	{"ISPPRE", BIT(2),		/* 0X0100 */ "TAA0",		BIT(5) | GENMASK(3, 0)},
	{"ISPPRE", GENMASK(2, 1),	/* 0X0110 */ "TAA1",		BIT(5) | GENMASK(3, 0)},
	{"ISPPRE", BIT(3),		/* 0X1000 */ "VPP",		BIT(5) | GENMASK(3, 0)},
	{"ISPPRE", BIT(0),		/* 0XXXX1 */ "SYSMMU_ISPPRE",	BIT(5) | BIT(0)},

	{"ISPLP0", 0,			/* 0XX000 */ "ISPLP",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP0", BIT(1),		/* 0XX010 */ "GDC",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP0", BIT(2),		/* 0XX100 */ "VRA2",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP0", BIT(0),		/* 0XXXX1 */ "SYSMMU_ISPLP0",	BIT(5) | BIT(0)},

	{"ISPLP1", 0,			/* 0XXXX0 */ "MC_SCALER",	BIT(5) | BIT(2) | BIT(0)},
	{"ISPLP1", BIT(0),		/* 0XXXX1 */ "SYSMMU_ISPLP1",	BIT(5) | BIT(0)},

	{"ISPHQ", 0,			/* 0XXXX0 */ "ISPHQ",		BIT(5) | BIT(0)},
	{"ISPHQ", BIT(0),		/* 0XXXX1 */ "SYSMMU_ISPHQ",	BIT(5) | BIT(0)},

	{"IVA", 0,			/* 0XXXX0 */ "IVA",		BIT(5) | BIT(0)},
	{"IVA", BIT(0),			/* 0XXXX1 */ "SYSMMU_IVA",	BIT(5) | BIT(0)},

	{"DSPM0", 0,			/* 0XXXX0 */ "SCOREAP3",	BIT(5) | BIT(0)},
	{"DSPM0", BIT(0),		/* 0XXXX1 */ "SYSMMU_DSPM0",	BIT(5) | BIT(0)},

	{"DSPM1", 0,			/* 0XXXX0 */ "SCOREAP3",	BIT(5) | BIT(0)},
	{"DSPM1", BIT(0),		/* 0XXXX1 */ "SYSMMU_DSPM1",	BIT(5) | BIT(0)},

	{"MFC0", 0,			/* 0XXXX0 */ "MFC0",		BIT(5) | BIT(0)},
	{"MFC0", BIT(0),		/* 0XXXX1 */ "SYSMMU_MFC0",	BIT(5) | BIT(0)},

	{"MFC1", 0,			/* 0XXX00 */ "MFC1",		BIT(5) | GENMASK(1, 0)},
	{"MFC1", BIT(1),		/* 0XXX10 */ "WFD",		BIT(5) | GENMASK(1, 0)},
	{"MFC1", BIT(0),		/* 0XXXX1 */ "SYSMMU_MFC1",	BIT(5) | BIT(0)},

	{"G2D0", 0,			/* 0XXXX0 */ "G2D0",		BIT(5) | BIT(0)},
	{"G2D0", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D0",	BIT(5) | BIT(0)},

	{"G2D1", 0,			/* 0XXXX0 */ "G2D1",		BIT(5) | BIT(0)},
	{"G2D1", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D1",	BIT(5) | BIT(0)},

	{"G2D2", 0,			/* 0XX000 */ "JPEG",		BIT(5) | GENMASK(2, 0)},
	{"G2D2", BIT(1),		/* 0XX010 */ "MSCL",		BIT(5) | GENMASK(2, 0)},
	{"G2D2", BIT(2),		/* 0XX100 */ "ASTC",		BIT(5) | GENMASK(2, 0)},
	{"G2D2", GENMASK(2, 1),		/* 0XX110 */ "JSQZ",		BIT(5) | GENMASK(2, 0)},
	{"G2D2", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D2",	BIT(5) | BIT(0)},

	{"AUD", 0,			/* 0XX000 */ "SPUS/SPUM",	BIT(5) | GENMASK(2, 0)},
	{"AUD", BIT(1),			/* 0XX010 */ "Cortex-A7",	BIT(5) | GENMASK(2, 0)},
	{"AUD", BIT(2),			/* 0XX100 */ "P_AUD",		BIT(5) | GENMASK(2, 0)},
	{"AUD", BIT(0),			/* 0XXXX1 */ "SYSMMU_ABOX",	BIT(5) | GENMASK(2, 0)},

	{"FSYS0", 0,			/* 0XXX10 */ "USB30DRD",	BIT(5) | GENMASK(1, 0)},
	{"FSYS0", BIT(0),		/* 0XXXX1 */ "PCIE_GEN3_A",	BIT(5) | GENMASK(1, 0)},
	{"FSYS0", BIT(1),		/* 0XXXX1 */ "PCIE_GEN3_B",	BIT(5) | GENMASK(1, 0)},

	{"FSYS1", 0,			/* 0XX000 */ "MMC_CARD",	BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(0),		/* 0X0001 */ "PCIE_GEN2",	BIT(5) | GENMASK(3, 0)},
	{"FSYS1", BIT(3) | BIT(0),	/* 0X1001 */ "SYSMMU_FSYS1(PCIE_GEN2)",	BIT(5) | GENMASK(3, 0)},
	{"FSYS1", BIT(1),		/* 0XX010 */ "UFS_EMBD",	BIT(5) | GENMASK(2, 0)},
	{"FSYS1", GENMASK(1, 0),	/* 0XX011 */ "SSS",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(2),		/* 0XX100 */ "RTIC",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(2) | BIT(0),	/* 0XX101 */ "UFS_CARD",	BIT(5) | GENMASK(2, 0)},

	{"AVPS", 0,			/* 000000 */ "ALIVE-APM",	GENMASK(5, 0)},
	{"AVPS", BIT(2),		/* 000100 */ "ALIVE-DBG_CORE",	GENMASK(5, 0)},
	{"AVPS", BIT(3),		/* 001000 */ "ALIVE-PEM",	GENMASK(5, 0)},
	{"AVPS", GENMASK(3, 2),		/* 001100 */ "ALIVE-P_APM",	GENMASK(5, 0)},
	{"AVPS", BIT(4),		/* 010000 */ "ALIVE-C_MODEM",	GENMASK(5, 0)},
	{"AVPS", GENMASK(4, 2),		/* 011100 */ "ALIVE-C_VTS",	GENMASK(5, 0)},
	{"AVPS", BIT(0),		/* 000001 */ "VTS-P_VTS or VTS-DMIC_IF_3RD_1",	GENMASK(5, 0)},
	{"AVPS", BIT(2) | BIT(0),	/* 000101 */ "VTS-LP_VTS",	GENMASK(5, 0)},
	{"AVPS", BIT(3) | BIT(0),	/* 001001 */ "VTS-DMIC_IF_0",	GENMASK(5, 0)},
	{"AVPS", GENMASK(3, 2) | BIT(0),/* 001101 */ "VTS-DMIC_IF_1",	GENMASK(5, 0)},
	{"AVPS", BIT(4) | BIT(0),	/* 010001 */ "VTS-CM4F_Data",	GENMASK(5, 0)},
	{"AVPS", BIT(4) | BIT(2) | BIT(0),/* 010101 */ "VTS-CM4F_Instruction", GENMASK(5, 0)},
	{"AVPS", GENMASK(4, 3) | BIT(0),/* 011001 */ "VTS-CM4F_System", GENMASK(5, 0)},
	{"AVPS", GENMASK(4, 2) | BIT(0),/* 011101 */ "VTS-DMIC_IF_3RD_0", GENMASK(5, 0)},
	{"AVPS", BIT(1),		/* 0XXX10 */ "SPDMA", 		BIT(5) | GENMASK(1, 0)},
	{"AVPS", GENMASK(1, 0),		/* 0XXX11 */ "PDMA", 		BIT(5) | GENMASK(1, 0)},

	{"CP_", 0,			/* 000000 */ "UCPUM",		GENMASK(5, 0)},
	{"CP_", BIT(0),			/* 000001 */ "LCPUM",		GENMASK(5, 0)},
	{"CP_", BIT(1),			/* 000010 */ "DMA0",		GENMASK(5, 0)},
	{"CP_", GENMASK(1, 0),		/* 000011 */ "DMA1",		GENMASK(5, 0)},
	{"CP_", BIT(2),			/* 000100 */ "DMA2",		GENMASK(5, 0)},
	{"CP_", BIT(2) | BIT(0),	/* 000101 */ "DATAMOVER",	GENMASK(5, 0)},
	{"CP_", GENMASK(2, 1),		/* 000110 */ "BAYES",		GENMASK(5, 0)},
	{"CP_", GENMASK(2, 0),		/* 000111 */ "LOGGER",		GENMASK(5, 0)},
	{"CP_", BIT(3),			/* 001000 */ "LMAC0",		GENMASK(5, 0)},
	{"CP_", BIT(3) | BIT(0),	/* 001001 */ "LMAC1",		GENMASK(5, 0)},
	{"CP_", BIT(3) | BIT(1),	/* 001010 */ "PktProc",		GENMASK(5, 0)},
	{"CP_", BIT(3) | GENMASK(1, 0),	/* 001011 */ "HARQMOVER0",	GENMASK(5, 0)},
	{"CP_", GENMASK(3, 2),		/* 001100 */ "CSXAP",		GENMASK(5, 0)},
	{"CP_", GENMASK(3, 2) | BIT(0),	/* 001101 */ "HARQMOVER1",	GENMASK(5, 0)},

	{"NPU", BIT(3),			/* 001000 */ "CM7",		GENMASK(5, 0)},
	{"NPU", GENMASK(3, 2),		/* 001100 */ "DMA330",		GENMASK(5, 0)},
	{"NPU", BIT(1),			/* 000010 */ "FMT",		BIT(5) | GENMASK(1, 0)},
	{"NPU", BIT(0),			/* 000010 */ "SYSMMU_NPU",	BIT(5) | BIT(0)},

	/* Others */
	{"SIREX", 0, "SIREX", 0},
	{"DIT", 0, "DIT", 0},
	{"SBIC", 0, "SBIC", 0},
	{"PDMA", 0, "PDMA", 0},
	{"SPDMA", 0, "SPDMA", 0},
	{"BUSC_PD", 0, "BUSC_PD", 0},
	{"VRA2", 0, "VRA2", 0},
	{"CORESIGHT", 0, "CORESIGHT", 0},
};


/* Data Path */
static struct itmon_nodeinfo data_bus_c[] = {
	{M_NODE,	"AUD",		0x1B173000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"AVPS",		0x1B133000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"BUSC_PD",	0x1B183000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DIT",		0x1B163000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DPU0",		0x1B003000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DPU1",		0x1B013000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DPU2",		0x1B023000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DSPM0",	0x1B0A3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"DSPM1",	0x1B0B3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"FSYS0",	0x1B113000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"FSYS1",	0x1B123000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"G2D0",		0x1B0E3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"G2D1",		0x1B0F3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"G2D2",		0x1B103000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"ISPHQ",	0x1B063000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"ISPLP0",	0x1B043000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"ISPLP1",	0x1B053000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"ISPPRE",	0x1B033000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"IVA",		0x1B093000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"MFC0",		0x1B0C3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"MFC1",		0x1B0D3000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"VRA2",		0x1B083000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"NPU",		0x1B073000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"SBIC",		0x1B153000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"SIREX",	0x1B143000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"BUSC_DP",	0x1B1E3000, NULL, TMOUT, true, false, true, true, false, NULL},
	{T_S_NODE,	"BUSC_S0",	0x1B193000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"BUSC_S1",	0x1B1A3000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"BUSC_S2",	0x1B1B3000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"BUSC_S3",	0x1B1C3000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"SP",		0x1B1D3000, NULL, TMOUT, true, false, true, true, false, NULL},
};

static struct itmon_nodeinfo data_bus_c1[] = {
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	{T_M_NODE,	"BUSC_M0",	0x1B403000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"BUSC_M1",	0x1B413000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"BUSC_M2",	0x1B423000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"BUSC_M3",	0x1B433000, NULL, 0,	false, false, false, false, false, NULL},
	{T_S_NODE,	"CORE_M0",	0x1B443000, NULL, 0,	false, false, false, false, false, NULL},
	{T_S_NODE,	"CORE_M1",	0x1B453000, NULL, 0,	false, false, false, false, false, NULL},
	{T_S_NODE,	"CORE_M2",	0x1B463000, NULL, 0,	false, false, false, false, false, NULL},
	{T_S_NODE,	"CORE_M3",	0x1B473000, NULL, 0,	false, false, false, false, false, NULL},
#else
	{T_M_NODE,	"BUSC_M0",	0x1B403000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"BUSC_M1",	0x1B413000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"BUSC_M2",	0x1B423000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"BUSC_M3",	0x1B433000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_M0",	0x1B443000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_M1",	0x1B453000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_M2",	0x1B463000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_M3",	0x1B473000, NULL, 0,	false, false, true, true, false, NULL},
#endif
};

static struct itmon_nodeinfo data_core[] = {
	{M_NODE,	"CORESIGHT",	0x1A863000, NULL, 0,	false, false, true, true, false, NULL},
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	{T_M_NODE,	"CORE_M0",	0x1A803000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"CORE_M1",	0x1A813000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"CORE_M2",	0x1A823000, NULL, 0,	false, false, false, false, false, NULL},
	{T_M_NODE,	"CORE_M3",	0x1A833000, NULL, 0,	false, false, false, false, false, NULL},
#else
	{T_M_NODE,	"CORE_M0",	0x1A803000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_M1",	0x1A813000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_M2",	0x1A823000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_M3",	0x1A833000, NULL, 0,	false, false, true, false, false, NULL},
#endif
	{M_NODE,	"CP_DATA_0",	0x1A843000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"CP_DATA_1",	0x1A853000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"CORE_DP",	0x1A893000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"DREX_IRPS0",	0x1A873000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"DREX_IRPS1",	0x1A883000, NULL, TMOUT, true, false, true, true, false, NULL},
};

static struct itmon_nodeinfo peri_core_0[] = {
	{M_NODE,	"CORE_DP",	0x1AA53000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"SCI_CCM0",	0x1AA23000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"SCI_CCM1",	0x1AA33000, NULL, 0,	false, false, true, true, false, NULL},
	{M_NODE,	"SCI_IRPM",	0x1AA43000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_BUSC",	0x1AA13000, NULL, 0,	false, false, true, true, false, NULL},
	{T_S_NODE,	"CORE_POP1",	0x1AA03000, NULL, 0,	false, false, true, true, false, NULL},
};

static struct itmon_nodeinfo peri_core_1[] = {
	{T_M_NODE,	"BUSC_CORE",	0x1AC13000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_POP1",	0x1AC03000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"ALIVE",	0x1AC83000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"CP_PERI",	0x1AC73000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"CPUCL0",	0x1AC43000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"CPUCL1",	0x1AC53000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"G3D",		0x1AC63000, NULL, TMOUT / 3, true, false, true, true, false, "pd-embedded_g3d",},
	{S_NODE,	"SFR_CORE",	0x1AC33000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"TREX_DP_CORE",	0x1AC23000, NULL, TMOUT, true, false, true, true, false, NULL},
};

static struct itmon_nodeinfo peri_bus_c[] = {
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	{M_NODE,	"BUSC_DP",	0x1B9B3000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_BUSC",	0x1B9C3000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"ABOX",		0x1B953000, NULL, TMOUT, true, false, true, true, false, "pd-aud"},
	{T_S_NODE,	"BUSC_CORE",	0x1B833000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"BUSC_PD",	0x1B9A3000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"DPU",		0x1B8B3000, NULL, TMOUT, true, false, true, true, false, "pd-dpu"},
	{S_NODE,	"DSPM",		0x1B923000, NULL, TMOUT, true, false, true, true, false, "pd-dspm"},
	{S_NODE,	"FSYS0",	0x1B963000, NULL, TMOUT, true, false, true, true, false, "pd-fsys0a"},
	{S_NODE,	"FSYS1",	0x1B973000, NULL, TMOUT, true, false, true, true, false, "pd-fsys1"},
	{S_NODE,	"G2D",		0x1B943000, NULL, TMOUT, true, false, true, true, false, "pd-g2d"},
	{S_NODE,	"ISPHQ",	0x1B8E3000, NULL, TMOUT, true, false, true, true, false, "pd-isphq"},
	{S_NODE,	"ISPLP",	0x1B8D3000, NULL, TMOUT, true, false, true, true, false, "pd-isplp"},
	{S_NODE,	"ISPPRE",	0x1B8C3000, NULL, TMOUT, true, false, true, true, false, "pd-isppre"},
	{S_NODE,	"IVA",		0x1B913000, NULL, TMOUT, true, false, true, true, false, "pd-iva"},
	{S_NODE,	"MFC",		0x1B933000, NULL, TMOUT, true, false, true, true, false, "pd-mfc"},
	{S_NODE,	"MIF0",		0x1B843000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF1",		0x1B853000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF2",		0x1B863000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF3",		0x1B873000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"NPU0",		0x1B8F3000, NULL, TMOUT, true, false, true, true, false, "pd-npu0"},
	{S_NODE,	"NPU1",		0x1B903000, NULL, TMOUT, true, false, true, true, false, "pd-npu1"},
	{S_NODE,	"PERIC0",	0x1B893000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"PERIC1",	0x1B8A3000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"PERICS",	0x1B883000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SFR0_BUSC",	0x1B813000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SFR1_BUSC",	0x1B823000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SIREX",	0x1B973000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"TREX_DP_BUSC",	0x1B803000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"VTS",		0x1B983000, NULL, TMOUT, true, false, true, true, false, "pd-vts"},
#else
	{M_NODE,	"BUSC_DP",	0x1B9A3000, NULL, 0,	false, false, true, true, false, NULL},
	{T_M_NODE,	"CORE_BUSC",	0x1B9B3000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"ABOX",		0x1B943000, NULL, TMOUT, true, false, true, true, false, "pd-aud"},
	{T_S_NODE,	"BUSC_CORE",	0x1B833000, NULL, 0,	false, false, true, true, false, NULL},
	{S_NODE,	"BUSC_PD",	0x1B993000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"DPU",		0x1B8B3000, NULL, TMOUT, true, false, true, true, false, "pd-dpu"},
	{S_NODE,	"DSPM",		0x1B913000, NULL, TMOUT, true, false, true, true, false, "pd-dspm"},
	{S_NODE,	"FSYS0",	0x1B953000, NULL, TMOUT, true, false, true, true, false, "pd-fsys0a"},
	{S_NODE,	"FSYS1",	0x1B963000, NULL, TMOUT, true, false, true, true, false, "pd-fsys1"},
	{S_NODE,	"G2D",		0x1B933000, NULL, TMOUT, true, false, true, true, false, "pd-g2d"},
	{S_NODE,	"ISPHQ",	0x1B8E3000, NULL, TMOUT, true, false, true, true, false, "pd-isphq"},
	{S_NODE,	"ISPLP",	0x1B8D3000, NULL, TMOUT, true, false, true, true, false, "pd-isplp"},
	{S_NODE,	"ISPPRE",	0x1B8C3000, NULL, TMOUT, true, false, true, true, false, "pd-isppre"},
	{S_NODE,	"IVA",		0x1B903000, NULL, TMOUT, true, false, true, true, false, "pd-iva"},
	{S_NODE,	"MFC",		0x1B923000, NULL, TMOUT, true, false, true, true, false, "pd-mfc"},
	{S_NODE,	"MIF0",		0x1B843000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF1",		0x1B853000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF2",		0x1B863000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"MIF3",		0x1B873000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"NPU",		0x1B8F3000, NULL, TMOUT, true, false, true, true, false, "pd-npu"},
	{S_NODE,	"PERIC0",	0x1B893000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"PERIC1",	0x1B8A3000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"PERICS",	0x1B883000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SFR0_BUSC",	0x1B813000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SFR1_BUSC",	0x1B823000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"SIREX",	0x1B973000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"TREX_DP_BUSC",	0x1B803000, NULL, TMOUT, true, false, true, true, false, NULL},
	{S_NODE,	"VTS",		0x1B983000, NULL, TMOUT, true, false, true, true, false, "pd-vts"},
#endif
};

static struct itmon_nodegroup nodegroup[] = {
	/* Data Path */
	{52,	"DATA_BUS_C",	0x1B3F3000, false, NULL, data_bus_c,	ARRAY_SIZE(data_bus_c), BUS_DATA},
	{53,	"DATA_BUS_C1",	0x1B4F3000, false, NULL, data_bus_c1,	ARRAY_SIZE(data_bus_c1),BUS_DATA},
	{121,	"DATA_CORE",	0x1A8F3000, false, NULL, data_core,	ARRAY_SIZE(data_core),	BUS_DATA},
	/* Peri Path */
	{127,	"PERI_CORE_0",	0x1AAF3000, false, NULL, peri_core_0,	ARRAY_SIZE(peri_core_0), BUS_PERI},
	{128,	"PERI_CORE_1",	0x1ACF3000, false, NULL, peri_core_1,	ARRAY_SIZE(peri_core_1), BUS_PERI},
	{54,	"PERI_BUS_C",	0x1B9F3000, false, NULL, peri_bus_c,	ARRAY_SIZE(peri_bus_c), BUS_PERI},
};

const static char *itmon_pathtype[] = {
	"DATA Path transaction (0x2000_0000 ~ 0xf_ffff_ffff)",
	"PERI(SFR) Path transaction (0x0 ~ 0x1fff_ffff)",
};

/* Error Code Description */
const static char *itmon_errcode[] = {
	"Error Detect by the Slave(SLVERR)",
	"Decode error(DECERR)",
	"Unsupported transaction error",
	"Power Down access error",
	"Unsupported transaction",
	"Unsupported transaction",
	"Timeout error - response timeout in timeout value",
	"Invalid errorcode",
};

const static char *itmon_nodestring[] = {
	"M_NODE",
	"TAXI_S_NODE",
	"TAXI_M_NODE",
	"S_NODE",
};

static struct itmon_dev *g_itmon;

/* declare notifier_list */
ATOMIC_NOTIFIER_HEAD(itmon_notifier_list);

static const struct of_device_id itmon_dt_match[] = {
	{.compatible = "samsung,exynos-itmon",
	 .data = NULL,},
	{},
};
MODULE_DEVICE_TABLE(of, itmon_dt_match);

static void itmon_switch_scandump(struct itmon_dev *itmon)
{
	/* TODO: scandump */
}

static void itmon_switch_s2d(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;

	if ((pdata->err_hwa || pdata->err_fatal) &&
		dbg_snapshot_get_debug_level() != DSS_DEBUG_LEVEL_LOW) {
		pr_info("ITMON Runs Scan2DRAM Trigger by err_hwa : %d, or err_fatal : %d\n",
			pdata->err_hwa, pdata->err_fatal);
		s3c2410wdt_set_emergency_reset(3, 0);
		dbg_snapshot_spin_func();
	}
}

static struct itmon_rpathinfo *itmon_get_rpathinfo(struct itmon_dev *itmon,
					       unsigned int id,
					       char *dest_name)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_rpathinfo *rpath = NULL;
	int i;

	if (!dest_name)
		return NULL;

	for (i = 0; i < (int)ARRAY_SIZE(rpathinfo); i++) {
		if (pdata->rpathinfo[i].id == (id & pdata->rpathinfo[i].bits)) {
			if (dest_name && !strncmp(pdata->rpathinfo[i].dest_name,
						  dest_name,
						  strlen(pdata->rpathinfo[i].dest_name))) {
				rpath = (struct itmon_rpathinfo *)&pdata->rpathinfo[i];
				break;
			}
		}
	}
	return rpath;
}

static struct itmon_masterinfo *itmon_get_masterinfo(struct itmon_dev *itmon,
						 char *port_name,
						 unsigned int user)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_masterinfo *master = NULL;
	unsigned int val;
	int i;

	if (!port_name)
		return NULL;

	for (i = 0; i < (int)ARRAY_SIZE(masterinfo); i++) {
		if (!strncmp(pdata->masterinfo[i].port_name, port_name, strlen(port_name))) {
			val = user & pdata->masterinfo[i].bits;
			if (val == pdata->masterinfo[i].user) {
				master = (struct itmon_masterinfo *)&pdata->masterinfo[i];
				break;
			}
		}
	}
	return master;
}

static void itmon_init(struct itmon_dev *itmon, bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node;
	unsigned int offset;
	int i, j;

	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		node = pdata->nodegroup[i].nodeinfo;
		for (j = 0; j < pdata->nodegroup[i].nodesize; j++) {
			if (node[j].type == S_NODE && node[j].tmout_enabled) {
				offset = OFFSET_TMOUT_REG;
				/* Enable Timeout setting */
				__raw_writel(enabled, node[j].regs + offset + REG_DBG_CTL);
				/* set tmout interval value */
				__raw_writel(node[j].time_val,
					     node[j].regs + offset + REG_TMOUT_INIT_VAL);
				pr_debug("Exynos ITMON - %s timeout enabled\n", node[j].name);
				if (node[j].tmout_frz_enabled) {
					/* Enable freezing */
					__raw_writel(enabled,
						     node[j].regs + offset + REG_TMOUT_FRZ_EN);
				}
			}
			if (node[j].err_enabled) {
				/* clear previous interrupt of req_read */
				offset = OFFSET_REQ_R;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of req_write */
				offset = OFFSET_REQ_W;
				if (pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of response_read */
				offset = OFFSET_RESP_R;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of response_write */
				offset = OFFSET_RESP_W;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);
				pr_debug("Exynos ITMON - %s error reporting enabled\n", node[j].name);
			}
			if (node[j].hw_assert_enabled) {
				offset = OFFSET_HW_ASSERT;
				__raw_writel(RD_RESP_INT_ENABLE | WR_RESP_INT_ENABLE |
					     ARLEN_RLAST_INT_ENABLE | AWLEN_WLAST_INT_ENABLE,
						node[j].regs + offset + REG_HWA_CTL);
			}
			if (node[j].addr_detect_enabled) {
				/* This feature is only for M_NODE */
				unsigned int tmp, val;

				offset = OFFSET_HW_ASSERT;
				val = __raw_readl(node[j].regs + offset + REG_HWA_CTL);
				val |= INTEND_ACCESS_INT_ENABLE;
				__raw_writel(val, node[j].regs + offset + REG_HWA_CTL);

				val = ((unsigned int)INTEND_ADDR_START & 0xFFFFFFFF);
				__raw_writel(val, node[j].regs + offset + REG_HWA_START_ADDR_LOW);
				val = (unsigned int)(((unsigned long)INTEND_ADDR_START >> 32) & 0XFFFF);
				__raw_writel(val, node[j].regs + offset + REG_HWA_START_END_ADDR_UPPER);

				val = ((unsigned int)INTEND_ADDR_END & 0xFFFFFFFF);
				__raw_writel(val, node[j].regs + offset + REG_HWA_END_ADDR_LOW);
				val = ((unsigned int)(((unsigned long)INTEND_ADDR_END >> 32) & 0XFFFF0000) << 16);
				tmp = readl(node[j].regs + offset + REG_HWA_START_END_ADDR_UPPER);
				__raw_writel(tmp | val, node[j].regs + offset + REG_HWA_START_END_ADDR_UPPER);
			}
		}
	}
}

void itmon_enable(bool enabled)
{
	if (g_itmon)
		itmon_init(g_itmon, enabled);
}

void itmon_set_errcnt(int cnt)
{
	struct itmon_platdata *pdata;

	if (g_itmon) {
		pdata = g_itmon->pdata;
		pdata->err_cnt = cnt;
	}
}

static void itmon_post_handler_to_notifier(struct itmon_dev *itmon,
					   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	/* After treatment by port */
	if (!traceinfo->port || strlen(traceinfo->port) < 1)
		return;

	itmon->notifier_info.port = traceinfo->port;
	itmon->notifier_info.master = traceinfo->master;
	itmon->notifier_info.dest = traceinfo->dest;
	itmon->notifier_info.read = traceinfo->read;
	itmon->notifier_info.target_addr = traceinfo->target_addr;
	itmon->notifier_info.errcode = traceinfo->errcode;
	itmon->notifier_info.onoff = traceinfo->onoff;
	itmon->notifier_info.pd_name = traceinfo->pd_name;

	pr_info("     +ITMON Notifier Call Information\n\n");

	/* call notifier_call_chain of itmon */
	atomic_notifier_call_chain(&itmon_notifier_list, 0, &itmon->notifier_info);

	pr_info("\n     -ITMON Notifier Call Information\n"
		"--------------------------------------------------------------------------\n");
}

static void itmon_post_handler_by_dest(struct itmon_dev *itmon,
					unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	if (!traceinfo->dest || strlen(traceinfo->dest) < 1)
		return;

	if (traceinfo->errcode == ERRCODE_TMOUT && traceinfo->snode_dirty == true) {
		if (!strncmp(traceinfo->dest, DREX_COMMON_STR, strlen(DREX_COMMON_STR)) ||
				!strncmp(traceinfo->dest, "CORE_DP", strlen("CORE_DP"))) {
				pdata->err_fatal = true;
		}
	}
}

static void itmon_post_handler_by_master(struct itmon_dev *itmon,
					unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	/* After treatment by port */
	if (!traceinfo->port || strlen(traceinfo->port) < 1)
		return;

	/*
	 * if master is CPU or SCI_CCM or SCI_IRPM,
	 * then we expect any exception at CPU
	 */
	if ((!strncmp(traceinfo->port, "CPU", strlen("CPU"))) ||
	    (!strncmp(traceinfo->port, "SCI_CCM", strlen("SCI_CCM"))) ||
	    (!strncmp(traceinfo->port, "SCI_IRPM", strlen("SCI_IRPM")))) {
		ktime_t now, interval;

		now = ktime_get();
		interval = ktime_sub(now, pdata->last_time);
		pdata->last_time = now;
		pdata->err_cnt_by_cpu++;
		if (traceinfo->errcode == ERRCODE_TMOUT &&
			traceinfo->snode_dirty == true) {
			pdata->err_cnt = PANIC_ALLOWED_THRESHOLD + 1;
			pr_info("ITMON try to run PANIC, even CPU transaction detected - %s",
			itmon_errcode[traceinfo->errcode]);
		} else {
			pr_info("ITMON skips CPU transaction detected - "
				"err_cnt_by_cpu: %u, interval: %lluns\n",
				pdata->err_cnt_by_cpu,
				(unsigned long long)ktime_to_ns(interval));
		}
	} else if (!strncmp(traceinfo->port, CP_COMMON_STR, strlen(CP_COMMON_STR))) {
		/* if master is DSP and operation is read, we don't care this */
		if (traceinfo->master && traceinfo->target_addr == INVALID_REMAPPING &&
			!strncmp(traceinfo->master, "LCPUM", strlen(traceinfo->master))) {
			pr_info("ITMON skips CP's LCPUM detected\n");
		} else {
			pr_info("CP Crash sequence in progress\n");
			/* Disable busmon all interrupts */
			itmon_init(itmon, false);
			pdata->crash_in_progress = true;
#if defined(CONFIG_SEC_SIPC_MODEM_IF)
			modem_force_crash_exit_ext();
#endif
		}
		pdata->err_fatal = false;
	} else {
		pdata->err_cnt++;
		pr_info("ITMON increases error count - err_cnt:%u\n", pdata->err_cnt);
	}
}

static void itmon_report_timeout(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	unsigned int id, payload, axid, user, valid, timeout, info;
	unsigned long addr;
	char *master_name, *port_name;
	struct itmon_rpathinfo *port;
	struct itmon_masterinfo *master;
	int i, num = (trans_type == TRANS_TYPE_READ ? SZ_128 : SZ_64);
	int rw_offset = (trans_type == TRANS_TYPE_READ ? 0 : REG_TMOUT_BUF_WR_OFFSET);

	pr_info("\n----------------------------------------------------------------------------------\n"
		"      ITMON Report (%s)\n"
		"----------------------------------------------------------------------------------\n"
		"      Timeout Error Occurred : Master --> %s (DRAM)\n\n",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE", node->name);
	pr_info("      TIMEOUT_BUFFER Information(NODE: %s)\n"
		"	> NUM|   BLOCK|  MASTER|VALID|TIMEOUT|      ID| PAYLOAD|   ADDRESS|   SRAM3|\n",
			node->name);

	for (i = 0; i < num; i++) {
		writel(i, node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_POINT_ADDR + rw_offset);
		id = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_ID + rw_offset);
		payload = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD + rw_offset);
		addr = (((unsigned long)readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_SRAM1 + rw_offset) &
				GENMASK(15, 0)) << 32ULL);
		addr |= (readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_SRAM2 + rw_offset));
		info = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_PAYLOAD_SRAM3 + rw_offset);
		/* ID[5:0] 6bit : R-PATH */
		axid = id & GENMASK(5, 0);
		/* PAYLOAD[15:8] : USER */
		user = (payload & GENMASK(15, 8)) >> 8;
		/* PAYLOAD[0] : Valid or Not valid */
		valid = payload & BIT(0);
		/* PAYLOAD[19:16] : Timeout */
		timeout = (payload & (unsigned int)(GENMASK(19, 16))) >> 16;

		port = (struct itmon_rpathinfo *)
				itmon_get_rpathinfo(itmon, axid, node->name);
		if (port) {
			port_name = port->port_name;
			master = (struct itmon_masterinfo *)
				itmon_get_masterinfo(itmon, port_name, user);
			if (master)
				master_name = master->master_name;
			else
				master_name = "Unknown";
		} else {
			port_name = "Unknown";
			master_name = "Unknown";
		}
		pr_info("      > %03d|%8s|%8s|%5u|%7x|%08x|%08X|%010zx|%08x|\n",
				i, port_name, master_name, valid, timeout, id, payload, addr, info);
	}
	pr_info("----------------------------------------------------------------------------------\n");
}

static unsigned int power(unsigned int param, unsigned int num)
{
	if (num == 0)
		return 1;
	return param * (power(param, num - 1));
}

static void itmon_report_powerinfo(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	static const char * const gpd_status_lookup[] = {
		[GPD_STATE_ACTIVE] = "on",
		[GPD_STATE_POWER_OFF] = "off"
	};
	static const char * const rpm_status_lookup[] = {
		[RPM_ACTIVE] = "active",
		[RPM_RESUMING] = "resuming",
		[RPM_SUSPENDED] = "suspended",
		[RPM_SUSPENDING] = "suspending"
	};
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct exynos_pm_domain *exynos_pd;
	struct generic_pm_domain *genpd;
	const char *p = "";
	struct pm_domain_data *pm_data;

	if (!traceinfo->dirty || !node->pd_name)
		return;

	exynos_pd = exynos_pd_lookup_name(node->pd_name);
	if (exynos_pd) {
		genpd = &exynos_pd->genpd;
		if (!genpd)
			return;
		pr_info("      Block Power Information - %s\n\n"
			"      > pd-domain name   : %s\n"
			"      > pd-domain status : %s\n\n"
			"--------------------------------------------------------------------------\n"
			"      Runtime-PM Information\n\n"
			"      > genpd name       : %s\n"
			"      > genpd status     : %s\n",
			traceinfo->dest ? traceinfo->dest : "Unknown",
			exynos_pd->name,
			cal_pd_status(exynos_pd->cal_pdid) ? "on" : "off",
			genpd->name,
			gpd_status_lookup[genpd->status]);

		list_for_each_entry(pm_data, &genpd->dev_list, list_node) {
			if (pm_data->dev->power.runtime_error)
				p = "error";
			else if (pm_data->dev->power.disable_depth)
				p = "unsupported";
			else if (pm_data->dev->power.runtime_status < ARRAY_SIZE(rpm_status_lookup))
				p = rpm_status_lookup[pm_data->dev->power.runtime_status];
			else
				WARN_ON(1);

			pr_info("      > dev name         : %s\n"
				"      > runtime-pm status: %s\n",
				dev_name(pm_data->dev), p);
		}
		traceinfo->onoff = cal_pd_status(exynos_pd->cal_pdid);
		traceinfo->pd_name = exynos_pd->name;
		pr_info("--------------------------------------------------------------------------\n");
	}
}

static void itmon_report_traceinfo(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct itmon_nodegroup *group = NULL;
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	char temp_buf[SZ_128];
#endif

	if (!traceinfo->dirty)
		return;

	pr_auto(ASL3,
		"--------------------------------------------------------------------------\n"
		"      Transaction Information\n\n"
		"      > Master         : %s %s\n"
		"      > Target         : %s\n"
		"      > Target Address : 0x%lX %s\n"
		"      > Type           : %s\n"
		"      > Error code     : %s\n",
		traceinfo->port, traceinfo->master ? traceinfo->master : "",
		traceinfo->dest ? traceinfo->dest : "Unknown",
		traceinfo->target_addr,
		(unsigned int)traceinfo->target_addr == INVALID_REMAPPING ?
		"(BAAW Remapped address)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[traceinfo->errcode]);
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	snprintf(temp_buf, SZ_128, "%s %s/ %s/ 0x%zx %s/ %s/ %s",
		traceinfo->port, traceinfo->master ? traceinfo->master : "",
		traceinfo->dest ? traceinfo->dest : "Unknown",
		traceinfo->target_addr,
		traceinfo->target_addr == INVALID_REMAPPING ?
		"(by CP maybe)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[traceinfo->errcode]);
	sec_debug_set_extra_info_busmon(temp_buf);
#endif

	if (node) {
		struct itmon_tracedata *tracedata = &node->tracedata;

		pr_auto(ASL3,
			"      > Size           : %u bytes x %u burst => %u bytes\n"
			"      > Burst Type     : %u (0:FIXED, 1:INCR, 2:WRAP)\n"
			"      > Level          : %s\n"
			"      > Protection     : %s\n",
			power(BIT_AXSIZE(tracedata->ext_info_1), 2), BIT_AXLEN(tracedata->ext_info_1) + 1,
			power(BIT_AXSIZE(tracedata->ext_info_1), 2) * (BIT_AXLEN(tracedata->ext_info_1) + 1),
			BIT_AXBURST(tracedata->ext_info_2),
			(BIT_AXPROT(tracedata->ext_info_2) & 0x1) ? "Privileged access" : "Unprivileged access",
			(BIT_AXPROT(tracedata->ext_info_2) & 0x2) ? "Non-secure access" : "Secure access");

		group = node->group;
		pr_auto(ASL3,
			"      > Path Type      : %s\n"
			"--------------------------------------------------------------------------\n",
			itmon_pathtype[traceinfo->path_type == -1 ? group->bus_type : traceinfo->path_type]);

	} else {
		pr_auto(ASL3, "--------------------------------------------------------------------------\n");
	}
}

static void itmon_report_pathinfo(struct itmon_dev *itmon,
				  struct itmon_nodeinfo *node,
				  unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *tracedata = &node->tracedata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	if (!traceinfo->path_dirty) {
		pr_auto(ASL3,
			"--------------------------------------------------------------------------\n"
			"      ITMON Report (%s)\n"
			"--------------------------------------------------------------------------\n"
			"      PATH Information\n",
			trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
		traceinfo->path_dirty = true;
	}
	switch (node->type) {
	case M_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "M_NODE", node->phy_regs + tracedata->offset);
		break;
	case T_S_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "T_S_NODE", node->phy_regs + tracedata->offset);
		break;
	case T_M_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "T_M_NODE", node->phy_regs + tracedata->offset);
		break;
	case S_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "S_NODE", node->phy_regs + tracedata->offset);
		break;
	}
}

static void itmon_report_tracedata(struct itmon_dev *itmon,
				   struct itmon_nodeinfo *node,
				   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *tracedata = &node->tracedata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct itmon_masterinfo *master;
	struct itmon_rpathinfo *port;
	unsigned int errcode, axid;
	unsigned int userbit;
	bool port_find;

	errcode = BIT_ERR_CODE(tracedata->int_info);
	axid = (unsigned int)BIT_AXID(tracedata->int_info);
	userbit = BIT_AXUSER(tracedata->ext_info_2);

	switch (node->type) {
	case M_NODE:
		/*
		 * In this case, we can get information from M_NODE
		 * Fill traceinfo->port / target_addr / read / master
		 */
		if (BIT_ERR_VALID(tracedata->int_info) && tracedata->ext_info_2) {
			/* If only detecting M_NODE only */
			traceinfo->port = node->name;
			master = (struct itmon_masterinfo *)
				itmon_get_masterinfo(itmon, node->name, userbit);
			if (master)
				traceinfo->master = master->master_name;
			else
				traceinfo->master = NULL;

			if (!strncmp(node->name, CP_COMMON_STR, strlen(CP_COMMON_STR)))
				set_bit(FROM_CP, &traceinfo->from);

			traceinfo->target_addr = (((unsigned long)node->tracedata.ext_info_1
								& GENMASK(3, 0)) << 32ULL);
			traceinfo->target_addr |= node->tracedata.ext_info_0;
			traceinfo->read = tracedata->read;
			traceinfo->errcode = errcode;
			traceinfo->dirty = true;
		} else {
			if (!strncmp(node->name, "SCI_IRPM", strlen(node->name)))
				set_bit(FROM_CPU, &traceinfo->from);
		}
		/* Pure M_NODE and it doesn't have any information */
		if (!traceinfo->dirty) {
			traceinfo->master = NULL;
			traceinfo->target_addr = 0;
			traceinfo->read = tracedata->read;
			traceinfo->port = node->name;
			traceinfo->errcode = errcode;
			traceinfo->dirty = true;
		}
		itmon_report_pathinfo(itmon, node, trans_type);
		break;
	case S_NODE:
		if (test_bit(FROM_CPU, &traceinfo->from)) {
			/*
			 * This case is slave error
			 * Master is CPU cluster
			 * user & GENMASK(1, 0) = core number
			 */
			int cluster_num, core_num;

			core_num = userbit & GENMASK(1, 0);
			cluster_num = (userbit & BIT(2)) >> 2;
			snprintf(traceinfo->buf, SZ_32 - 1, "CPU%d Cluster%d", core_num, cluster_num);
			traceinfo->port = traceinfo->buf;
		} else if (test_bit(FROM_CP, &traceinfo->from) && (!traceinfo->port))
			traceinfo->port = CP_COMMON_STR;

		/* S_NODE = BUSC_DP or CORE_DP => PERI */
		if (!strncmp(node->name, "BUSC_DP", strlen(node->name)) ||
			!strncmp(node->name, "CORE_DP", strlen(node->name))) {
			port_find = true;
			traceinfo->path_type = BUS_PERI;
		} else if (!strncmp(node->name, DREX_COMMON_STR, strlen(DREX_COMMON_STR))) {
			port_find = true;
			traceinfo->path_type = BUS_DATA;
		} else {
			port_find = false;
			traceinfo->path_type = -1;
		}
		if (port_find && !test_bit(FROM_CPU, &traceinfo->from)) {
			port = (struct itmon_rpathinfo *)
				itmon_get_rpathinfo(itmon, axid, node->name);
			/* If it couldn't find port, keep previous information */
			if (port) {
				traceinfo->port = port->port_name;
				master = (struct itmon_masterinfo *)
						itmon_get_masterinfo(itmon, traceinfo->port,
							userbit);
				if (master)
					traceinfo->master = master->master_name;
			}
		} else {
			/* If it has traceinfo->port, keep previous information */
			if (!traceinfo->port) {
				port = (struct itmon_rpathinfo *)
						itmon_get_rpathinfo(itmon, axid, node->name);
				if (port) {
					if (!strncmp(port->port_name, CP_COMMON_STR,
							strlen(CP_COMMON_STR)))
						traceinfo->port = CP_COMMON_STR;
					else
						traceinfo->port = port->port_name;
				}
			}
			if (!traceinfo->master && traceinfo->port) {
				master = (struct itmon_masterinfo *)
						itmon_get_masterinfo(itmon, traceinfo->port,
							userbit);
				if (master)
					traceinfo->master = master->master_name;
			}
		}
		/* Update targetinfo with S_NODE */
		traceinfo->target_addr =
			(((unsigned long)node->tracedata.ext_info_1
			& GENMASK(3, 0)) << 32ULL);
		traceinfo->target_addr |= node->tracedata.ext_info_0;
		traceinfo->errcode = errcode;
		traceinfo->dest = node->name;
		traceinfo->dirty = true;
		traceinfo->snode_dirty = true;
		itmon_report_pathinfo(itmon, node, trans_type);
		itmon_report_traceinfo(itmon, node, trans_type);
		itmon_report_powerinfo(itmon, node, trans_type);
		break;
	case T_S_NODE:
	case T_M_NODE:
		itmon_report_pathinfo(itmon, node, trans_type);
		break;
	default:
		pr_info("Unknown Error - offset:%u\n", tracedata->offset);
		break;
	}
}

static void itmon_report_hwa_rawdata(struct itmon_dev *itmon,
				     struct itmon_nodeinfo *node)
{
	unsigned int dbg_mo_cnt, hwa_ctl, hwa_info, hwa_int_id;

	dbg_mo_cnt = __raw_readl(node->regs +  OFFSET_HW_ASSERT);
	hwa_ctl = __raw_readl(node->regs +  OFFSET_HW_ASSERT + REG_HWA_CTL);
	hwa_info = __raw_readl(node->regs +  OFFSET_HW_ASSERT + REG_HWA_INT);
	hwa_int_id = __raw_readl(node->regs + OFFSET_HW_ASSERT + REG_HWA_INT_ID);

	/* Output Raw register information */
	pr_info("--------------------------------------------------------------------------\n"
		"      HWA Raw Register Information(ITMON information)\n\n");
	pr_info("      > %s(%s, 0x%08X)\n"
		"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		node->name, itmon_nodestring[node->type],
		node->phy_regs,
		dbg_mo_cnt,
		hwa_ctl,
		hwa_info,
		hwa_int_id);
}

static void itmon_report_rawdata(struct itmon_dev *itmon,
				 struct itmon_nodeinfo *node,
				 unsigned int trans_type)
{
	struct itmon_tracedata *tracedata = &node->tracedata;

	/* Output Raw register information */
	pr_info("      > %s(%s, 0x%08X)\n"
		"      > REG(0x08~0x18)        : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n"
		"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		node->name, itmon_nodestring[node->type],
		node->phy_regs + tracedata->offset,
		tracedata->int_info,
		tracedata->ext_info_0,
		tracedata->ext_info_1,
		tracedata->ext_info_2,
		tracedata->dbg_mo_cnt,
		tracedata->hwa_ctl,
		tracedata->hwa_info,
		tracedata->hwa_int_id);
}

static void itmon_route_tracedata(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo;
	struct itmon_nodeinfo *node, *next_node;
	unsigned int trans_type;
	int i;

	/* To call function is sorted by declaration */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_each_entry(node, &pdata->tracelist[trans_type], list) {
				if (i == node->type)
					itmon_report_tracedata(itmon, node, trans_type);
			}
		}
		/* If there is no S_NODE information, check one more */
		traceinfo = &pdata->traceinfo[trans_type];
		if (!traceinfo->snode_dirty)
			itmon_report_traceinfo(itmon, NULL, trans_type);
	}

	if (pdata->traceinfo[TRANS_TYPE_READ].dirty ||
		pdata->traceinfo[TRANS_TYPE_WRITE].dirty)
		pr_auto(ASL3, " Raw Register Information(ITMON Internal Information)\n\n");

	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_each_entry_safe(node, next_node, &pdata->tracelist[trans_type], list) {
				if (i == node->type) {
					itmon_report_rawdata(itmon, node, trans_type);
					/* clean up */
					list_del(&node->list);
					kfree(node);
				}
			}
		}
	}

	if (pdata->traceinfo[TRANS_TYPE_READ].dirty ||
		pdata->traceinfo[TRANS_TYPE_WRITE].dirty)
		pr_auto(ASL3, "--------------------------------------------------------------------------\n");

	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		itmon_post_handler_to_notifier(itmon, trans_type);
		itmon_post_handler_by_dest(itmon, trans_type);
		itmon_post_handler_by_master(itmon, trans_type);
	}
}

static void itmon_trace_data(struct itmon_dev *itmon,
			    struct itmon_nodegroup *group,
			    struct itmon_nodeinfo *node,
			    unsigned int offset)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *new_node = NULL;
	unsigned int int_info, info0, info1, info2;
	unsigned int hwa_ctl, hwa_info, hwa_int_id, dbg_mo_cnt;
	bool read = TRANS_TYPE_WRITE;
	bool req = false;

	int_info = __raw_readl(node->regs + offset + REG_INT_INFO);
	info0 = __raw_readl(node->regs + offset + REG_EXT_INFO_0);
	info1 = __raw_readl(node->regs + offset + REG_EXT_INFO_1);
	info2 = __raw_readl(node->regs + offset + REG_EXT_INFO_2);

	dbg_mo_cnt = __raw_readl(node->regs +  OFFSET_HW_ASSERT);
	hwa_ctl = __raw_readl(node->regs +  OFFSET_HW_ASSERT + REG_HWA_CTL);
	hwa_info = __raw_readl(node->regs +  OFFSET_HW_ASSERT + REG_HWA_INT);
	hwa_int_id = __raw_readl(node->regs + OFFSET_HW_ASSERT + REG_HWA_INT_ID);

	switch (offset) {
	case OFFSET_REQ_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_REQ_W:
		req = true;
		/* Only S-Node is able to make log to registers */
		break;
	case OFFSET_RESP_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_RESP_W:
		req = false;
		/* Only NOT S-Node is able to make log to registers */
		break;
	default:
		pr_auto(ASL3, "Unknown Error - node:%s offset:%u\n", node->name, offset);
		break;
	}

	new_node = kmalloc(sizeof(struct itmon_nodeinfo), GFP_ATOMIC);
	if (new_node) {
		/* Fill detected node information to tracedata's list */
		memcpy(new_node, node, sizeof(struct itmon_nodeinfo));
		new_node->tracedata.int_info = int_info;
		new_node->tracedata.ext_info_0 = info0;
		new_node->tracedata.ext_info_1 = info1;
		new_node->tracedata.ext_info_2 = info2;
		new_node->tracedata.dbg_mo_cnt = dbg_mo_cnt;
		new_node->tracedata.hwa_ctl = hwa_ctl;
		new_node->tracedata.hwa_info = hwa_info;
		new_node->tracedata.hwa_int_id = hwa_int_id;

		new_node->tracedata.offset = offset;
		new_node->tracedata.read = read;
		new_node->group = group;
		if (BIT_ERR_VALID(int_info))
			node->tracedata.logging = true;
		else
			node->tracedata.logging = false;

		list_add(&new_node->list, &pdata->tracelist[read]);
	} else {
		pr_auto(ASL3, "failed to kmalloc for %s node %x offset\n",
			node->name, offset);
	}
}

static int itmon_search_node(struct itmon_dev *itmon, struct itmon_nodegroup *group, bool clear)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = NULL;
	unsigned int val, offset, freeze;
	unsigned long vec, flags, bit = 0;
	int i, j, ret = 0;

	spin_lock_irqsave(&itmon->ctrl_lock, flags);
	memset(pdata->traceinfo, 0, sizeof(struct itmon_traceinfo) * 2);
	ret = 0;

	if (group) {
		/* Processing only this group and select detected node */
		if (group->phy_regs) {
			if (group->ex_table)
				vec = (unsigned long)__raw_readq(group->regs);
			else
				vec = (unsigned long)__raw_readl(group->regs);
		} else {
			vec = GENMASK(group->nodesize, 0);
		}

		if (!vec)
			goto exit;

		node = group->nodeinfo;
		for_each_set_bit(bit, &vec, group->nodesize) {
			/* exist array */
			for (i = 0; i < OFFSET_NUM; i++) {
				offset = i * OFFSET_ERR_REPT;
				/* Check Request information */
				val = __raw_readl(node[bit].regs + offset + REG_INT_INFO);
				if (BIT_ERR_OCCURRED(val)) {
					/* This node occurs the error */
					itmon_trace_data(itmon, group, &node[bit], offset);
					if (clear)
						__raw_writel(1, node[bit].regs
								+ offset + REG_INT_CLR);
					ret = true;
				}
			}
			/* Check H/W assertion */
			if (node[bit].hw_assert_enabled) {
				val = __raw_readl(node[bit].regs + OFFSET_HW_ASSERT +
							REG_HWA_INT);
				/* if timeout_freeze is enable,
				 * HWA interrupt is able to assert without any information */
				if (BIT_HWA_ERR_OCCURRED(val) && (val & GENMASK(31, 1))) {
					itmon_report_hwa_rawdata(itmon, &node[bit]);
					pdata->err_hwa = true;
					/* Go panic now */
					pdata->err_cnt = PANIC_ALLOWED_THRESHOLD + 1;
					ret = true;
				}
			}
			/* Check freeze enable node */
			if (node[bit].type == S_NODE && node[bit].tmout_frz_enabled) {
				val = __raw_readl(node[bit].regs + OFFSET_TMOUT_REG  +
							REG_TMOUT_BUF_STATUS);
				freeze = val & (unsigned int)GENMASK(1, 0);
				if (freeze) {
					if (freeze & BIT(0))
						itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_WRITE);
					if (freeze & BIT(1))
						itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_READ);
					pdata->err_fatal = true;
					/* Go panic now */
					pdata->err_cnt = PANIC_ALLOWED_THRESHOLD + 1;
					ret = true;
				}
			}
		}
	} else {
		/* Processing all group & nodes */
		for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
			group = &nodegroup[i];
			if (group->phy_regs) {
				if (group->ex_table)
					vec = (unsigned long)__raw_readq(group->regs);
				else
					vec = (unsigned long)__raw_readl(group->regs);
			} else {
				vec = GENMASK(group->nodesize, 0);
			}

			node = group->nodeinfo;
			bit = 0;

			for_each_set_bit(bit, &vec, group->nodesize) {
				for (j = 0; j < OFFSET_NUM; j++) {
					offset = j * OFFSET_ERR_REPT;
					/* Check Request information */
					val = __raw_readl(node[bit].regs + offset + REG_INT_INFO);
					if (BIT_ERR_OCCURRED(val)) {
						/* This node occurs the error */
						itmon_trace_data(itmon, group, &node[bit], offset);
						if (clear)
							__raw_writel(1, node[bit].regs
									+ offset + REG_INT_CLR);
						ret = true;
					}
				}
				/* Check H/W assertion */
				if (node[bit].hw_assert_enabled) {
					val = __raw_readl(node[bit].regs + OFFSET_HW_ASSERT +
								REG_HWA_INT);
					/* if timeout_freeze is enable,
					 * HWA interrupt is able to assert without any information */
					if (BIT_HWA_ERR_OCCURRED(val) && (val & GENMASK(31, 1))) {
						itmon_report_hwa_rawdata(itmon, &node[bit]);
						pdata->err_hwa = true;
						/* Go panic now */
						pdata->err_cnt = PANIC_ALLOWED_THRESHOLD + 1;
						ret = true;
					}
				}
				/* Check freeze enable node */
				if (node[bit].type == S_NODE && node[bit].tmout_frz_enabled) {
					val = __raw_readl(node[bit].regs + OFFSET_TMOUT_REG  +
								REG_TMOUT_BUF_STATUS);
					freeze = val & (unsigned int)(GENMASK(1, 0));
					if (freeze) {
						if (freeze & BIT(0))
							itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_WRITE);
						if (freeze & BIT(1))
							itmon_report_timeout(itmon, &node[bit], TRANS_TYPE_READ);
						pdata->err_fatal = true;
						/* Go panic now */
						pdata->err_cnt = PANIC_ALLOWED_THRESHOLD + 1;
						ret = true;
					}
				}
			}
		}
	}
	itmon_route_tracedata(itmon);
 exit:
	spin_unlock_irqrestore(&itmon->ctrl_lock, flags);
	return ret;
}

static irqreturn_t itmon_irq_handler(int irq, void *data)
{
	struct itmon_dev *itmon = (struct itmon_dev *)data;
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodegroup *group = NULL;
	bool ret;
	int i;

	/* Search itmon group */
	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		if (irq == nodegroup[i].irq) {
			group = &pdata->nodegroup[i];
			if (group->phy_regs != 0) {
				pr_info("\nITMON Detected: %d irq, %s group, 0x%x vec, "
					"err_cnt:%u err_cnt_by_cpu:%u\n",
					irq, group->name,
					__raw_readl(group->regs),
					pdata->err_cnt,
					pdata->err_cnt_by_cpu);
			} else {
				pr_info("\nITMON Detected: %d irq, %s group, "
					"err_cnt:%u err_cnt_by_cpu:%u\n",
					irq, group->name,
					pdata->err_cnt,
					pdata->err_cnt_by_cpu);
			}
			break;
		}
	}

	ret = itmon_search_node(itmon, NULL, true);
	if (!ret) {
		pr_info("ITMON could not detect any error\n");
	} else {
		if (pdata->sysfs_scandump) {
			itmon_switch_scandump(itmon);
		} else if (pdata->sysfs_s2d) {
			itmon_switch_s2d(itmon);
		}

		if (pdata->err_cnt > PANIC_ALLOWED_THRESHOLD)
			pdata->panic_allowed = true;
	}

	if (pdata->panic_allowed)
		panic("ITMON occurs panic, Transaction is invalid from IPs");

	return IRQ_HANDLED;
}

void itmon_notifier_chain_register(struct notifier_block *block)
{
	atomic_notifier_chain_register(&itmon_notifier_list, block);
}

static struct bus_type itmon_subsys = {
	.name = "itmon",
	.dev_name = "itmon",
};

static ssize_t itmon_timeout_fix_val_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct itmon_platdata *pdata = g_itmon->pdata;

	n = scnprintf(buf + n, 24, "set timeout val: 0x%x \n", pdata->sysfs_tmout_val);

	return n;
}

static ssize_t itmon_timeout_fix_val_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val = 0;
	int ret = 0;
	struct itmon_platdata *pdata = g_itmon->pdata;

	ret = kstrtoul(buf, 16, &val);
	if (!ret && val > 0UL && val <= 0xFFFFFUL)
		pdata->sysfs_tmout_val = val;

	return count;
}

static ssize_t itmon_scandump_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct itmon_platdata *pdata = g_itmon->pdata;

	n = scnprintf(buf + n, 24, "scandump mode is %sable : %d\n",
		pdata->sysfs_scandump == 1 ? "en" : "dis",
		pdata->sysfs_scandump);

	return n;
}

static ssize_t itmon_scandump_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val = 0;

	kstrtoul(buf, 16, &val);
	g_itmon->pdata->sysfs_scandump = !!val;

	return count;
}

static ssize_t itmon_s2d_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct itmon_platdata *pdata = g_itmon->pdata;

	n = scnprintf(buf + n, 24, "s2d mode is %sable : %d\n",
		pdata->sysfs_s2d == 1 ? "en" : "dis",
		pdata->sysfs_s2d);

	return n;
}

static ssize_t itmon_s2d_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val = 0;

	kstrtoul(buf, 16, &val);
	g_itmon->pdata->sysfs_s2d = !!val;

	return count;
}

static ssize_t itmon_timeout_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	unsigned long i, offset;
	ssize_t n = 0;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;

	/* Processing all group & nodes */
	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE) {
				n += scnprintf(buf + n, 60, "%-12s : 0x%08X, timeout : %x\n",
					node[bit].name, node[bit].phy_regs,
					__raw_readl(node[bit].regs + offset + REG_DBG_CTL));
			}
		}
	}
	return n;
}

static ssize_t itmon_timeout_val_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	unsigned long i, offset;
	ssize_t n = 0;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;

	/* Processing all group & nodes */
	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE) {
				n += scnprintf(buf + n, 60, "%-12s : 0x%08X, timeout : 0x%x\n",
					node[bit].name, node[bit].phy_regs,
					__raw_readl(node[bit].regs + offset + REG_TMOUT_INIT_VAL));
			}
		}
	}
	return n;
}

static ssize_t itmon_timeout_freeze_show(struct kobject *kobj,
			         struct kobj_attribute *attr, char *buf)
{
	unsigned long i, offset;
	ssize_t n = 0;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;

	/* Processing all group & nodes */
	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE) {
				n += scnprintf(buf + n, 60, "%-12s : 0x%08X, timeout_freeze : %x\n",
					node[bit].name, node[bit].phy_regs,
					__raw_readl(node[bit].regs + offset + REG_TMOUT_FRZ_EN));
			}
		}
	}
	return n;
}

static ssize_t itmon_timeout_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	char *name;
	unsigned int val, offset, i;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;

	name = (char *)kstrndup(buf, count, GFP_KERNEL);
	if (!name)
		return count;

	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE &&
				!strncmp(name, node[bit].name, strlen(name))) {
				val = __raw_readl(node[bit].regs + offset + REG_DBG_CTL);
				if (!val)
					val = 1;
				else
					val = 0;
				__raw_writel(val, node[bit].regs + offset + REG_DBG_CTL);
				node[bit].tmout_enabled = val;
			}
		}
	}
	kfree(name);
	return count;
}

static ssize_t itmon_timeout_val_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	char *name;
	unsigned int offset, i;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;
	struct itmon_platdata *pdata = g_itmon->pdata;

	name = (char *)kstrndup(buf, count, GFP_KERNEL);
	if (!name)
		return count;

	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE &&
				!strncmp(name, node[bit].name, strlen(name))) {
				__raw_writel(pdata->sysfs_tmout_val,
						node[bit].regs + offset + REG_TMOUT_INIT_VAL);
				node[bit].time_val = pdata->sysfs_tmout_val;
			}
		}
	}
	kfree(name);
	return count;
}

static ssize_t itmon_timeout_freeze_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	char *name;
	unsigned int val, offset, i;
	unsigned long vec, bit = 0;
	struct itmon_nodegroup *group = NULL;
	struct itmon_nodeinfo *node;

	name = (char *)kstrndup(buf, count, GFP_KERNEL);
	if (!name)
		return count;

	offset = OFFSET_TMOUT_REG;
	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		group = &nodegroup[i];
		node = group->nodeinfo;
		vec = GENMASK(group->nodesize, 0);
		bit = 0;
		for_each_set_bit(bit, &vec, group->nodesize) {
			if (node[bit].type == S_NODE &&
				!strncmp(name, node[bit].name, strlen(name))) {
				val = __raw_readl(node[bit].regs + offset + REG_TMOUT_FRZ_EN);
				if (!val)
					val = 1;
				else
					val = 0;
				__raw_writel(val, node[bit].regs + offset + REG_TMOUT_FRZ_EN);
				node[bit].tmout_frz_enabled = val;
			}
		}
	}
	kfree(name);
	return count;
}

static struct kobj_attribute itmon_timeout_attr =
        __ATTR(timeout_en, 0644, itmon_timeout_show, itmon_timeout_store);
static struct kobj_attribute itmon_timeout_fix_attr =
        __ATTR(set_val, 0644, itmon_timeout_fix_val_show, itmon_timeout_fix_val_store);
static struct kobj_attribute itmon_scandump_attr =
        __ATTR(scandump_en, 0644, itmon_scandump_show, itmon_scandump_store);
static struct kobj_attribute itmon_s2d_attr =
        __ATTR(s2d_en, 0644, itmon_s2d_show, itmon_s2d_store);
static struct kobj_attribute itmon_timeout_val_attr =
        __ATTR(timeout_val, 0644, itmon_timeout_val_show, itmon_timeout_val_store);
static struct kobj_attribute itmon_timeout_freeze_attr =
        __ATTR(timeout_freeze, 0644, itmon_timeout_freeze_show, itmon_timeout_freeze_store);

static struct attribute *itmon_sysfs_attrs[] = {
	&itmon_timeout_attr.attr,
	&itmon_timeout_fix_attr.attr,
	&itmon_timeout_val_attr.attr,
	&itmon_timeout_freeze_attr.attr,
	&itmon_scandump_attr.attr,
	&itmon_s2d_attr.attr,
	NULL,
};

static struct attribute_group itmon_sysfs_group = {
	.attrs = itmon_sysfs_attrs,
};

static const struct attribute_group *itmon_sysfs_groups[] = {
	&itmon_sysfs_group,
	NULL,
};

static int __init itmon_sysfs_init(void)
{
	int ret = 0;

	ret = subsys_system_register(&itmon_subsys, itmon_sysfs_groups);
	if (ret)
		pr_err("fail to register exynos-snapshop subsys\n");

	return ret;
}
late_initcall(itmon_sysfs_init);

static int itmon_logging_panic_handler(struct notifier_block *nb,
				     unsigned long l, void *buf)
{
	struct itmon_panic_block *itmon_panic = (struct itmon_panic_block *)nb;
	struct itmon_dev *itmon = itmon_panic->pdev;
	struct itmon_platdata *pdata = itmon->pdata;
	int ret;

	if (!IS_ERR_OR_NULL(itmon)) {
		/* Check error has been logged */
		ret = itmon_search_node(itmon, NULL, false);
		if (!ret) {
			pr_info("No found error in %s\n", __func__);
		} else {
			pr_info("Found errors in %s\n", __func__);
			if (pdata->sysfs_scandump) {
				itmon_switch_scandump(itmon);
			} else if (pdata->sysfs_s2d) {
				itmon_switch_s2d(itmon);
			}

		}
	}
	return 0;
}

static int itmon_probe(struct platform_device *pdev)
{
	struct itmon_dev *itmon;
	struct itmon_panic_block *itmon_panic = NULL;
	struct itmon_platdata *pdata;
	struct itmon_nodeinfo *node;
	unsigned int irq_option = 0, irq;
	char *dev_name;
	int ret, i, j;

	itmon = devm_kzalloc(&pdev->dev, sizeof(struct itmon_dev), GFP_KERNEL);
	if (!itmon)
		return -ENOMEM;

	itmon->dev = &pdev->dev;

	spin_lock_init(&itmon->ctrl_lock);

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct itmon_platdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	itmon->pdata = pdata;
	itmon->pdata->masterinfo = masterinfo;
	itmon->pdata->rpathinfo = rpathinfo;
	itmon->pdata->nodegroup = nodegroup;

	for (i = 0; i < (int)ARRAY_SIZE(nodegroup); i++) {
		dev_name = nodegroup[i].name;
		node = nodegroup[i].nodeinfo;

		if (nodegroup[i].phy_regs) {
			nodegroup[i].regs = devm_ioremap_nocache(&pdev->dev,
							 nodegroup[i].phy_regs, SZ_16K);
			if (nodegroup[i].regs == NULL) {
				dev_err(&pdev->dev, "failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}
#ifdef MULTI_IRQ_SUPPORT_ITMON
		irq_option = IRQF_GIC_MULTI_TARGET;
#endif
		irq = irq_of_parse_and_map(pdev->dev.of_node, i);
		nodegroup[i].irq = irq;

		ret = devm_request_irq(&pdev->dev, irq,
				       itmon_irq_handler, irq_option, dev_name, itmon);
		if (ret == 0) {
			dev_info(&pdev->dev, "success to register request irq%u - %s\n", irq, dev_name);
		} else {
			dev_err(&pdev->dev, "failed to request irq - %s\n", dev_name);
			return -ENOENT;
		}

		for (j = 0; j < nodegroup[i].nodesize; j++) {
			node[j].regs = devm_ioremap_nocache(&pdev->dev, node[j].phy_regs, SZ_16K);
			if (node[j].regs == NULL) {
				dev_err(&pdev->dev, "failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}
	}

	itmon_panic = devm_kzalloc(&pdev->dev, sizeof(struct itmon_panic_block),
				 GFP_KERNEL);

	if (!itmon_panic) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				    "panic handler data\n");
	} else {
		itmon_panic->nb_panic_block.notifier_call = itmon_logging_panic_handler;
		itmon_panic->pdev = itmon;
		atomic_notifier_chain_register(&panic_notifier_list,
					       &itmon_panic->nb_panic_block);
	}

	platform_set_drvdata(pdev, itmon);

	for (i = 0; i < TRANS_TYPE_NUM; i++)
		INIT_LIST_HEAD(&pdata->tracelist[i]);

	pdata->crash_in_progress = false;
	pdata->sysfs_s2d = true;

	itmon_init(itmon, true);

	g_itmon = itmon;
	pdata->probed = true;

	dev_info(&pdev->dev, "success to probe Exynos ITMON driver\n");

	return 0;
}

static int itmon_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int itmon_suspend(struct device *dev)
{
	return 0;
}

static int itmon_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct itmon_dev *itmon = platform_get_drvdata(pdev);
	struct itmon_platdata *pdata = itmon->pdata;

	/* re-enable ITMON if cp-crash progress is not starting */
	if (!pdata->crash_in_progress)
		itmon_init(itmon, true);

	return 0;
}

static SIMPLE_DEV_PM_OPS(itmon_pm_ops, itmon_suspend, itmon_resume);
#define ITMON_PM	(itmon_pm_ops)
#else
#define ITM_ONPM	NULL
#endif

static struct platform_driver exynos_itmon_driver = {
	.probe = itmon_probe,
	.remove = itmon_remove,
	.driver = {
		   .name = "exynos-itmon",
		   .of_match_table = itmon_dt_match,
		   .pm = &itmon_pm_ops,
		   },
};

module_platform_driver(exynos_itmon_driver);

MODULE_DESCRIPTION("Samsung Exynos ITMON DRIVER");
MODULE_AUTHOR("Hosung Kim <hosung0.kim@samsung.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:exynos-itmon");
