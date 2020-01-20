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
#define EXYNOS9810_PA_DPU0	0x1B000000
#define EXYNOS9810_PA_DPU1	0x1B010000
#define EXYNOS9810_PA_DPU2	0x1B020000
#define EXYNOS9810_PA_ISPPRE	0x1B030000
#define EXYNOS9810_PA_ISPLP0	0x1B040000
#define EXYNOS9810_PA_ISPLP1	0x1B050000
#define EXYNOS9810_PA_ISPHQ	0x1B060000
#define EXYNOS9810_PA_DCRD	0x1B070000
#define EXYNOS9810_PA_DCF	0x1B080000
#define EXYNOS9810_PA_IVA	0x1B090000
#define EXYNOS9810_PA_DSPM0	0x1B0A0000
#define EXYNOS9810_PA_DSPM1	0x1B0B0000
#define EXYNOS9810_PA_DSPM2	0x1B0C0000
#define EXYNOS9810_PA_MFC0	0x1B0D0000
#define EXYNOS9810_PA_MFC1	0x1B0E0000
#define EXYNOS9810_PA_G2D0	0x1B0F0000
#define EXYNOS9810_PA_G2D1	0x1B100000
#define EXYNOS9810_PA_G2D2	0x1B110000
#define EXYNOS9810_PA_AUD	0x1B120000
#define EXYNOS9810_PA_FSYS0	0x1B130000
#define EXYNOS9810_PA_FSYS1	0x1B140000
#define EXYNOS9810_PA_BUS1_D0	0x1B150000
#define EXYNOS9810_PA_SIREX	0x1B160000
#define EXYNOS9810_PA_SBIC	0x1B170000
#define EXYNOS9810_PA_PDMA	0x1B180000
#define EXYNOS9810_PA_SPDMA	0x1B190000
#define EXYNOS9810_PA_TREX_PD	0x1B1A0000
#define EXYNOS9810_PA_G3D0	0x1A840000
#define EXYNOS9810_PA_G3D1	0x1A850000
#define EXYNOS9810_PA_G3D2	0x1A860000
#define EXYNOS9810_PA_G3D3	0x1A870000
#define EXYNOS9810_PA_CP	0x1A880000

#define EXYNOS9810_PA_SCI	0x1A000000

#define EXYNOS9810_PA_SCI_IRPS0	0x1A894000
#define EXYNOS9810_PA_SCI_IRPS1	0x1A8A4000

#define EXYNOS9810_PA_SMC0	0x1B830000
#define EXYNOS9810_PA_SMC1	0x1B930000
#define EXYNOS9810_PA_SMC2	0x1BA30000
#define EXYNOS9810_PA_SMC3	0x1BB30000

#define EXYNOS9810_PA_SN_IRPS0	0x1A892000
#define EXYNOS9810_PA_SN_IRPS1	0x1A8A2000

#define EXYNOS9810_PA_SN_BUSC_M0	0x1B1B2000
#define EXYNOS9810_PA_SN_BUSC_M1	0x1B1C2000
#define EXYNOS9810_PA_SN_BUSC_M2	0x1B1D2000
#define EXYNOS9810_PA_SN_BUSC_M3	0x1B1E2000

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
