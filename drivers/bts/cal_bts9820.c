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

#include "cal_bts9820.h"
#include <linux/soc/samsung/exynos-soc.h>

#define LOG(x, ...)				\
({						\
	seq_printf(buf, x, ##__VA_ARGS__);	\
})

/* for BTS V3.0 Register */

#define TREX_CON				0x000
#define TREX_TIMEOUT				0x010
#define TREX_BLOCK_IDMASK			0x018
#define TREX_BLOCK_IDVALUE			0x01C
#define TREX_RCON				0x020
#define TREX_WCON				0x040
#define TREX_RBLOCK_UPPER			0x024
#define TREX_WBLOCK_UPPER			0x044
#define TREX_RBLOCK_UPPER_NORMAL		0x028
#define TREX_WBLOCK_UPPER_NORMAL		0x048
#define TREX_RBLOCK_UPPER_FULL			0x02C
#define TREX_WBLOCK_UPPER_FULL			0x04C
#define TREX_RBLOCK_UPPER_BUSY			0x030
#define TREX_WBLOCK_UPPER_BUSY			0x050
#define TREX_RBLOCK_UPPER_MAX			0x034
#define TREX_WBLOCK_UPPER_MAX			0x054

/* trex qos register */

#define SCI_CTRL				0x0000
#define TH_IMM_R_0				0x0100
#define TH_IMM_R_1				0x0104
#define TH_IMM_R_2				0x0108
#define TH_IMM_R_3				0x010C
#define TH_IMM_R_4				0x0110
#define TH_IMM_R_5				0x0114
#define TH_IMM_R_6				0x0118
#define TH_IMM_R_7				0x011C
#define TH_IMM_R_8				0x0120
#define TH_IMM_R_9				0x0124
#define TH_IMM_R_10				0x0128
#define TH_IMM_R_11				0x012C
#define TH_IMM_R_12				0x0130
#define TH_IMM_R_13				0x0134
#define TH_IMM_W_0				0x0180
#define TH_IMM_W_1				0x0184
#define TH_IMM_W_2				0x0188
#define TH_IMM_W_3				0x018C
#define TH_IMM_W_4				0x0190
#define TH_IMM_W_5				0x0194
#define TH_IMM_W_6				0x0198
#define TH_IMM_W_7				0x019C
#define TH_IMM_W_8				0x01A0
#define TH_IMM_W_9				0x01A4
#define TH_IMM_W_10				0x01A8
#define TH_IMM_W_11				0x01AC
#define TH_IMM_W_12				0x01B0
#define TH_IMM_W_13				0x01B4
#define TH_HIGH_R_0				0x0200
#define TH_HIGH_R_1				0x0204
#define TH_HIGH_R_2				0x0208
#define TH_HIGH_R_3				0x020C
#define TH_HIGH_R_4				0x0210
#define TH_HIGH_R_5				0x0214
#define TH_HIGH_R_6				0x0218
#define TH_HIGH_R_7				0x021C
#define TH_HIGH_R_8				0x0220
#define TH_HIGH_R_9				0x0224
#define TH_HIGH_R_10				0x0228
#define TH_HIGH_R_11				0x022C
#define TH_HIGH_R_12				0x0230
#define TH_HIGH_R_13				0x0234
#define TH_HIGH_W_0				0x0280
#define TH_HIGH_W_1				0x0284
#define TH_HIGH_W_2				0x0288
#define TH_HIGH_W_3				0x028C
#define TH_HIGH_W_4				0x0290
#define TH_HIGH_W_5				0x0294
#define TH_HIGH_W_6				0x0298
#define TH_HIGH_W_7				0x029C
#define TH_HIGH_W_8				0x02A0
#define TH_HIGH_W_9				0x02A4
#define TH_HIGH_W_10				0x02A8
#define TH_HIGH_W_11				0x02AC
#define TH_HIGH_W_12				0x02B0
#define TH_HIGH_W_13				0x02B4

#define QMAX_THRESHOLD_R			0x0050
#define QMAX_THRESHOLD_W			0x0054

#define SCI_CCMCMDTOKENS0			0x00fc
#define SCI_CCMCMDTOKENS1			0x0110
#define SCI_CCMTOKENRELEASECTL			0x0104
#define SCI_CRPCONTROL00			0x0188
#define SCI_CRPCONTROL01			0x01B0
#define SCI_CRPCONTROL02			0x01D8
#define SCI_CRPCONTROL03			0x0200

#define SCI_CRP0_CON				0x0204
#define SCI_CRP1_CON				0x022C
#define SCI_CRP2_CON				0x0254

#define SMC_SCHEDCTL_BUNDLE_CTRL4		0x0258

#define MAX_IDQ					0x7

static unsigned int set_mo(unsigned int mo)
{
	if (mo > BTS_MAX_MO || !mo)
		mo = BTS_MAX_MO;
	return mo;
}

void bts_setqos(void __iomem *base, struct bts_status *stat)
{
	unsigned int tmp_reg = 0;
	bool block_en = false;

	if (!base || !stat)
		return;

	if (stat->disable) {
		__raw_writel(0x4000, base + TREX_RCON);
		__raw_writel(0x4000, base + TREX_WCON);
		__raw_writel(0x0, base + TREX_CON);
		return;
	}
	__raw_writel(set_mo(stat->rmo), base + TREX_RBLOCK_UPPER);
	__raw_writel(set_mo(stat->wmo), base + TREX_WBLOCK_UPPER);
	if (stat->max_rmo || stat->max_wmo || stat->busy_rmo || stat->busy_wmo)
		block_en = true;
	__raw_writel(set_mo(stat->max_rmo),
		     base + TREX_RBLOCK_UPPER_MAX);
	__raw_writel(set_mo(stat->max_wmo),
		     base + TREX_WBLOCK_UPPER_MAX);
	__raw_writel(set_mo(stat->full_rmo),
		     base + TREX_RBLOCK_UPPER_FULL);
	__raw_writel(set_mo(stat->full_wmo),
		     base + TREX_WBLOCK_UPPER_FULL);
	__raw_writel(set_mo(stat->busy_rmo),
		     base + TREX_RBLOCK_UPPER_BUSY);
	__raw_writel(set_mo(stat->busy_wmo),
		     base + TREX_WBLOCK_UPPER_BUSY);
	if (stat->timeout_en) {
		if (stat->timeout_r > 0xff)
			stat->timeout_r = 0xff;
		if (stat->timeout_w > 0xff)
			stat->timeout_w = 0xff;
		__raw_writel(stat->timeout_r | (stat->timeout_w << 16),
			     base + TREX_TIMEOUT);
	} else {
		__raw_writel(0xff | (0xff << 16),
			     base + TREX_TIMEOUT);
	}
	/* override QoS value */
	tmp_reg |= (1 & !stat->bypass_en) << 8;
	tmp_reg |= (stat->priority & 0xf) << 12;
	/* enable Blocking logic */
	tmp_reg |= (1 & block_en) << 0;
	__raw_writel(tmp_reg, base + TREX_RCON);
	__raw_writel(tmp_reg, base + TREX_WCON);

	__raw_writel(((1 & stat->timeout_en) << 20) | 0x1, base + TREX_CON);
}

void bts_showqos(void __iomem *base, struct seq_file *buf)
{
	if (!base)
		return;

	LOG("CON0x%08X qos(%d,%d)0x%Xr%Xw TO(%d)0x%04Xr%04Xw \
	    MO(%d,%d)0x%04xr%04xwQbusy0x%04xr%04xwQfull0x%04xr%04xw \
	    Qmax0x%04xr%04xw\n",
	    __raw_readl(base + TREX_CON),
	    (__raw_readl(base + TREX_RCON) >> 8) & 0x1,
	    (__raw_readl(base + TREX_WCON) >> 8) & 0x1,
	    (__raw_readl(base + TREX_RCON) >> 12) & 0xf,
	    (__raw_readl(base + TREX_WCON) >> 12) & 0xf,
	    (__raw_readl(base + TREX_CON) >> 20) & 0x1,
	    (__raw_readl(base + TREX_TIMEOUT)) & 0xff,
	    (__raw_readl(base + TREX_TIMEOUT) >> 16) & 0xff,
	    __raw_readl(base + TREX_RCON) & 0x1,
	    __raw_readl(base + TREX_WCON) & 0x1,
	    __raw_readl(base + TREX_RBLOCK_UPPER),
	    __raw_readl(base + TREX_WBLOCK_UPPER),
	    __raw_readl(base + TREX_RBLOCK_UPPER_BUSY),
	    __raw_readl(base + TREX_WBLOCK_UPPER_BUSY),
	    __raw_readl(base + TREX_RBLOCK_UPPER_FULL),
	    __raw_readl(base + TREX_WBLOCK_UPPER_FULL),
	    __raw_readl(base + TREX_RBLOCK_UPPER_MAX),
	    __raw_readl(base + TREX_WBLOCK_UPPER_MAX));
}

void bts_trex_init(void __iomem *base)
{
	if (!base)
		return;

	/* high [27:24] mid [19:16] threshold */
	__raw_writel(0x0B070000, base + SCI_CTRL);

	__raw_writel(0x10101010, base + TH_IMM_R_0);
	__raw_writel(0x00101010, base + TH_IMM_R_1);
	__raw_writel(0x00001000, base + TH_IMM_R_2);
	__raw_writel(0x00000000, base + TH_IMM_R_3);
	__raw_writel(0x10101000, base + TH_IMM_R_4);
	__raw_writel(0x10101010, base + TH_IMM_R_5);
	__raw_writel(0x10101010, base + TH_IMM_R_6);
	__raw_writel(0x10101010, base + TH_IMM_R_7);
	__raw_writel(0x10000010, base + TH_IMM_R_8);
	__raw_writel(0x00000000, base + TH_IMM_R_9);
	__raw_writel(0x10000000, base + TH_IMM_R_10);
	__raw_writel(0x10101010, base + TH_IMM_R_11);
	__raw_writel(0x10101010, base + TH_IMM_R_12);
	__raw_writel(0x10101010, base + TH_IMM_R_13);
	__raw_writel(0x10101010, base + TH_IMM_W_0);
	__raw_writel(0x00101010, base + TH_IMM_W_1);
	__raw_writel(0x00001000, base + TH_IMM_W_2);
	__raw_writel(0x00000000, base + TH_IMM_W_3);
	__raw_writel(0x10101000, base + TH_IMM_W_4);
	__raw_writel(0x10101010, base + TH_IMM_W_5);
	__raw_writel(0x10101010, base + TH_IMM_W_6);
	__raw_writel(0x10101010, base + TH_IMM_W_7);
	__raw_writel(0x10000010, base + TH_IMM_W_8);
	__raw_writel(0x00000000, base + TH_IMM_W_9);
	__raw_writel(0x10000000, base + TH_IMM_W_10);
	__raw_writel(0x10101010, base + TH_IMM_W_11);
	__raw_writel(0x10101010, base + TH_IMM_W_12);
	__raw_writel(0x10101010, base + TH_IMM_W_13);

	__raw_writel(0x08080808, base + TH_HIGH_R_0);
	__raw_writel(0x00080808, base + TH_HIGH_R_1);
	__raw_writel(0x00000800, base + TH_HIGH_R_2);
	__raw_writel(0x00000000, base + TH_HIGH_R_3);
	__raw_writel(0x08080800, base + TH_HIGH_R_4);
	__raw_writel(0x08080808, base + TH_HIGH_R_5);
	__raw_writel(0x08080808, base + TH_HIGH_R_6);
	__raw_writel(0x08080808, base + TH_HIGH_R_7);
	__raw_writel(0x08000008, base + TH_HIGH_R_8);
	__raw_writel(0x00000000, base + TH_HIGH_R_9);
	__raw_writel(0x08000000, base + TH_HIGH_R_10);
	__raw_writel(0x08080808, base + TH_HIGH_R_11);
	__raw_writel(0x08080808, base + TH_HIGH_R_12);
	__raw_writel(0x08080808, base + TH_HIGH_R_13);
	__raw_writel(0x08080808, base + TH_HIGH_W_0);
	__raw_writel(0x00080808, base + TH_HIGH_W_1);
	__raw_writel(0x00000800, base + TH_HIGH_W_2);
	__raw_writel(0x00000000, base + TH_HIGH_W_3);
	__raw_writel(0x08080800, base + TH_HIGH_W_4);
	__raw_writel(0x08080808, base + TH_HIGH_W_5);
	__raw_writel(0x08080808, base + TH_HIGH_W_6);
	__raw_writel(0x08080808, base + TH_HIGH_W_7);
	__raw_writel(0x08000008, base + TH_HIGH_W_8);
	__raw_writel(0x00000000, base + TH_HIGH_W_9);
	__raw_writel(0x08000000, base + TH_HIGH_W_10);
	__raw_writel(0x08080808, base + TH_HIGH_W_11);
	__raw_writel(0x08080808, base + TH_HIGH_W_12);
	__raw_writel(0x08080808, base + TH_HIGH_W_13);
}

void bts_set_qmax(void __iomem *base, unsigned int rmo, unsigned int wmo)
{
	__raw_writel(set_mo(rmo), base + QMAX_THRESHOLD_R);
	__raw_writel(set_mo(wmo), base + QMAX_THRESHOLD_W);
}

void bts_set_qbusy(void __iomem *base, unsigned int qbusy)
{
	unsigned int tmp_reg = 0;

	if (!base)
		return;

	if (!qbusy)
		return;

	if (qbusy > 0x3f)
		qbusy = 0x3f;

	tmp_reg = __raw_readl(base + SMC_SCHEDCTL_BUNDLE_CTRL4);
	tmp_reg &= ~(0x3f << 16);
	tmp_reg &= ~(0x3f << 24);
	tmp_reg |= qbusy << 16;
	tmp_reg |= qbusy << 24;
	__raw_writel(tmp_reg, base + SMC_SCHEDCTL_BUNDLE_CTRL4);
}

void bts_set_qfull(void __iomem *base, unsigned int qfull_low,
		   unsigned int qfull_high)
{
	unsigned int tmp_reg = 0;

	if (!base)
		return;

	if (qfull_low == 0 && qfull_high == 0)
		return;

	if (qfull_low > 0x3f)
		qfull_low = 0x3f;
	if (qfull_high < qfull_low)
		qfull_high = qfull_low;
	else if (qfull_high > 0x3f)
		qfull_high = 0x3f;
	tmp_reg = __raw_readl(base + SCI_CCMTOKENRELEASECTL);
	tmp_reg &= ~(0x3f << 6);
	tmp_reg &= ~(0x3f << 12);
	tmp_reg |= qfull_high << 6;
	tmp_reg |= qfull_low << 12;
	__raw_writel(tmp_reg, base + SCI_CCMTOKENRELEASECTL);
}

void bts_show_idq(void __iomem *base, struct seq_file *buf)
{
	if (!base)
		return;

	LOG("IDQ[0] 0x%X\nIDQ[1] 0x%X\nIDQ[2] 0x%X\n",
		__raw_readl(base + SCI_CRPCONTROL00),
		__raw_readl(base + SCI_CRPCONTROL01),
		__raw_readl(base + SCI_CRPCONTROL02));
}


void bts_set_idq(void __iomem *base, unsigned int port,
			unsigned int idq)
{
	unsigned int tmp_reg = 0;
	unsigned int offset = 0;

	if (!base || idq > MAX_IDQ)
		return;

	switch (port) {
	case 0:
		offset = SCI_CRPCONTROL00;
		break;
	case 1:
		offset = SCI_CRPCONTROL01;
		break;
	case 2:
		offset = SCI_CRPCONTROL02;
		break;
	case 3:
		offset = SCI_CRPCONTROL03;
		break;
	default:
		pr_err("[BTS]: Invalid input parameter\n");
		return;
	}

	tmp_reg = __raw_readl(base + offset);
	tmp_reg &= ~(0x7 << 20);
	tmp_reg |= idq << 20;
	__raw_writel(tmp_reg, base + offset);

	return;
}
