/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * EXYNOS - BTS CAL code
 *
 */

#ifndef __BTSCAL_H__
#define __BTSCAL_H__

#include <linux/io.h>
#include <linux/debugfs.h>

/* For BTS Through SYSTEM Register */
#define EXYNOS9820_PA_CP0	0x1A840000
#define EXYNOS9820_PA_CP1	0x1A850000
#define EXYNOS9820_PA_DPU0	0x1B000000
#define EXYNOS9820_PA_DPU1	0x1B010000
#define EXYNOS9820_PA_DPU2	0x1B020000
#define EXYNOS9820_PA_ISPPRE	0x1B030000
#define EXYNOS9820_PA_ISPLP0	0x1B040000
#define EXYNOS9820_PA_ISPLP1	0x1B050000
#define EXYNOS9820_PA_ISPHQ	0x1B060000
#define EXYNOS9820_PA_NPU	0x1B070000
#ifndef CONFIG_SOC_EXYNOS9820_EVT0
#define EXYNOS9820_PA_VRA2	0x1B080000
#endif
#define EXYNOS9820_PA_IVA	0x1B090000
#define EXYNOS9820_PA_DSPM0	0x1B0A0000
#define EXYNOS9820_PA_DSPM1	0x1B0B0000
#define EXYNOS9820_PA_MFC0	0x1B0C0000
#define EXYNOS9820_PA_MFC1	0x1B0D0000
#define EXYNOS9820_PA_G2D0	0x1B0E0000
#define EXYNOS9820_PA_G2D1	0x1B0F0000
#define EXYNOS9820_PA_G2D2	0x1B100000
#define EXYNOS9820_PA_FSYS0	0x1B110000
#define EXYNOS9820_PA_FSYS1	0x1B120000
#define EXYNOS9820_PA_AVPS	0x1B130000
#define EXYNOS9820_PA_SIREX	0x1B140000
#define EXYNOS9820_PA_SBIC	0x1B150000
#define EXYNOS9820_PA_DIT	0x1B160000
#define EXYNOS9820_PA_AUD	0x1B170000
#define EXYNOS9820_PA_TREX_PD	0x1B180000

#define EXYNOS9820_PA_PDMA	0x1A340000
#define EXYNOS9820_PA_SPDMA	0x1A330000
#define EXYNOS9820_PA_G3D0	0x18490000
#define EXYNOS9820_PA_G3D1	0x184A0000
#define EXYNOS9820_PA_G3D2	0x184B0000
#define EXYNOS9820_PA_G3D3	0x184C0000

#define EXYNOS9820_PA_SCI	0x1A000000

#define EXYNOS9820_PA_SCI_IRPS0	0x1A874000
#define EXYNOS9820_PA_SCI_IRPS1	0x1A884000

#define EXYNOS9820_PA_SMC0	0x1BC30000
#define EXYNOS9820_PA_SMC1	0x1BD30000
#define EXYNOS9820_PA_SMC2	0x1BE30000
#define EXYNOS9820_PA_SMC3	0x1BF30000

#define EXYNOS9820_PA_SN_BUSC_S0	0x1B192000
#define EXYNOS9820_PA_SN_BUSC_S1	0x1B1A2000
#define EXYNOS9820_PA_SN_BUSC_S2	0x1B1B2000
#define EXYNOS9820_PA_SN_BUSC_S3	0x1B1C2000

#define BTS_MAX_MO		0xFFFF

struct bts_status {
	bool scen_en;
	unsigned int priority;
	bool disable;
	bool bypass_en;
	bool timeout_en;
	unsigned int rmo;
	unsigned int wmo;
	unsigned int full_rmo;
	unsigned int full_wmo;
	unsigned int busy_rmo;
	unsigned int busy_wmo;
	unsigned int max_rmo;
	unsigned int max_wmo;
	unsigned int timeout_r;
	unsigned int timeout_w;
	unsigned int idq;
};

void bts_setqos(void __iomem *base, struct bts_status *stat);
void bts_showqos(void __iomem *base, struct seq_file *buf);
void bts_trex_init(void __iomem *base);
void bts_set_qmax(void __iomem *base, unsigned int rmo, unsigned int wmo);
void bts_set_qbusy(void __iomem *base, unsigned int qbusy);
void bts_set_qfull(void __iomem *base, unsigned int qfull_low,
		   unsigned int qfull_high);
void bts_show_idq(void __iomem *base, struct seq_file *buf);
void bts_set_idq(void __iomem *base, unsigned int port,
			unsigned int idq);

#endif
