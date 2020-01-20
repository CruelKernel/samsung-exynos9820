/* sound/soc/samsung/abox/abox_effect.h
 *
 * ALSA SoC Audio Layer - Samsung Abox Effect driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_EFFECT_H
#define __SND_SOC_ABOX_EFFECT_H
enum {
	SOUNDALIVE = 0,
	MYSOUND,
	PLAYSPEED,
	SOUNDBALANCE,
	MYSPACE,
	BASSBOOST,
	EQUALIZER,
	NXPBUNDLE, /* includes BB,EQ, virtualizer, volume */
	NXPREVERB_CTX, /* Reverb context inforamtion */
	NXPREVERB_PARAM, /* Reverb effect parameters */
	SOUNDBOOSTER,
};

/* Effect offset */
#define SA_BASE			(0x000)
#define SA_CHANGE_BIT		(0x000)
#define SA_OUT_DEVICE		(0x010)
#define	SA_PRESET		(0x014)
#define SA_EQ_BEGIN		(0x018)
#define SA_EQ_END		(0x038)
#define SA_3D_LEVEL		(0x03C)
#define SA_BE_LEVEL		(0x040)
#define SA_REVERB		(0x044)
#define SA_ROOMSIZE		(0x048)
#define SA_CLA_LEVEL		(0x04C)
#define SA_VOLUME_LEVEL		(0x050)
#define SA_SQUARE_ROW		(0x054)
#define SA_SQUARE_COLUMN	(0x058)
#define SA_TAB_INFO		(0x05C)
#define SA_NEW_UI		(0x060)
#define SA_3D_ON		(0x064)
#define SA_3D_ANGLE_L		(0x068)
#define SA_3D_ANGLE_R		(0x06C)
#define SA_3D_GAIN_L		(0x070)
#define SA_3D_GAIN_R		(0x074)
#define SA_MAX_COUNT		(26)
#define SA_VALUE_MIN		(0)
#define SA_VALUE_MAX		(100)

#define MYSOUND_BASE		(0x100)
#define MYSOUND_CHANGE_BIT	(0x100)
#define MYSOUND_DHA_ENABLE	(0x110)
#define MYSOUND_GAIN_BEGIN	(0x114)
#define MYSOUND_OUT_DEVICE	(0x148)
#define MYSOUND_MAX_COUNT	(14)
#define MYSOUND_VALUE_MIN	(0)
#define MYSOUND_VALUE_MAX	(100)

#define VSP_BASE		(0x200)
#define VSP_CHANGE_BIT		(0x200)
#define VSP_INDEX		(0x210)
#define VSP_MAX_COUNT		(2)
#define VSP_VALUE_MIN		(0)
#define VSP_VALUE_MAX		(1000)

#define LRSM_BASE		(0x300)
#define LRSM_CHANGE_BIT		(0x300)
#define LRSM_INDEX0		(0x310)
#define LRSM_INDEX1		(0x320)
#define LRSM_MAX_COUNT		(3)
#define LRSM_VALUE_MIN		(-50)
#define LRSM_VALUE_MAX		(50)

#define MYSPACE_BASE		(0x340)
#define MYSPACE_CHANGE_BIT	(0x340)
#define MYSPACE_PRESET		(0x350)
#define MYSPACE_MAX_COUNT	(2)
#define MYSPACE_VALUE_MIN	(0)
#define MYSPACE_VALUE_MAX	(5)

#define BB_BASE			(0x400)
#define BB_CHANGE_BIT		(0x400)
#define BB_STATUS		(0x410)
#define BB_STRENGTH		(0x414)
#define BB_MAX_COUNT		(2)
#define BB_VALUE_MIN		(0)
#define BB_VALUE_MAX		(100)

#define EQ_BASE			(0x500)
#define EQ_CHANGE_BIT		(0x500)
#define EQ_STATUS		(0x510)
#define EQ_PRESET		(0x514)
#define EQ_NBAND		(0x518)
#define EQ_NBAND_LEVEL		(0x51c)	/* 10 nband levels */
#define EQ_NBAND_FREQ		(0x544)	/* 10 nband frequencies */
#define EQ_MAX_COUNT		(23)
#define EQ_VALUE_MIN		(0)
#define EQ_VALUE_MAX		(14000)

/* CommBox ELPE Parameter */
#define ELPE_BASE		(0x600)
#define ELPE_CMD		(0x600)
#define ELPE_ARG0		(0x604)
#define ELPE_ARG1		(0x608)
#define ELPE_ARG2		(0x60C)
#define ELPE_ARG3		(0x610)
#define ELPE_ARG4		(0x614)
#define ELPE_ARG5		(0x618)
#define ELPE_ARG6		(0x61C)
#define ELPE_ARG7		(0x620)
#define ELPE_ARG8		(0x624)
#define ELPE_ARG9		(0x628)
#define ELPE_ARG10		(0x62C)
#define ELPE_ARG11		(0x630)
#define ELPE_ARG12		(0x634)
#define ELPE_RET		(0x638)
#define ELPE_DONE		(0x63C)
#define ELPE_MAX_COUNT		(11)

/* NXP Bundle control parameters */
#define NXPBDL_BASE		(0x700)
#define NXPBDL_CHANGE_BIT	(0x700)
#define NXPBDL_MAX_COUNT	(35) /* bundle common struct param count */
#define NXPBDL_VALUE_MIN	(0)
#define NXPBDL_VALUE_MAX	(192000)

/* NXP Reverb control parameters */
#define NXPRVB_PARAM_BASE	(0x800)
#define NXPRVB_PARAM_CHANGE_BIT	(0x800)
#define NXPRVB_PARAM_MAX_COUNT	(10) /* reverb common struct param count */
#define NXPRVB_PARAM_VALUE_MIN	(0)
#define NXPRVB_PARAM_VALUE_MAX	(192000)

/* NXP reverb context parameters */
#define NXPRVB_CTX_BASE		(0x880)
#define NXPRVB_CTX_CHANGE_BIT	(0x880)
#define NXPRVB_CTX_MAX_COUNT	(7) /* reverb context param count */
#define NXPRVB_CTX_VALUE_MIN	(0)
#define NXPRVB_CTX_VALUE_MAX	(192000)

#define SB_BASE			(0x900)
#define SB_CHANGE_BIT		(0x900)
#define SB_ROTATION		(0x910)
#define SB_MAX_COUNT		(1)
#define SB_VALUE_MIN		(0)
#define SB_VALUE_MAX		(3)

#define UPSCALER_BASE		(0xA00)
#define UPSCALER_CHANGE_BIT	(0xA00)
#define UPSCALER_ROTATION	(0xA10)
#define UPSCALER_MAX_COUNT	(1)
#define UPSCALER_VALUE_MIN	(0)
#define UPSCALER_VALUE_MAX	(2)

#define DA_DATA_BASE		(0xB00)
#define DA_DATA_CHANGE_BIT	(0xB00)
#define DA_DATA			(0xB10)
#define DA_DATA_MAX_COUNT	(3)
#define DA_DATA_VALUE_MIN	(-1)
#define DA_DATA_VALUE_MAX	(15)

#define ABOX_EFFECT_MAX_REGISTERS	(0xB50)

#define PARAM_OFFSET		(0x10)
#define CHANGE_BIT		(1)

struct abox_effect_data {
	struct platform_device *pdev;
	void __iomem *base;
	struct regmap *regmap;
};

/**
 * Restore abox effect register map
 */
extern void abox_effect_restore(void);

#endif /* __SND_SOC_ABOX_EFFECT_H */
