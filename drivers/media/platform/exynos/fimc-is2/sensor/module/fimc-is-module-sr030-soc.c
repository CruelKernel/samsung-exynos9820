/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-hw.h"
#include "fimc-is-module-sr030-soc.h"

#define SENSOR_NAME "SR030"

struct sensor_reg {
	u8 addr;
	u8 data;
};

struct sensor_regset {
	struct sensor_reg	*reg;
	u32			size;
};

struct sensor_regset_table {
	struct sensor_regset init;
	struct sensor_regset effect_none;
	struct sensor_regset wb_auto;
	struct sensor_regset brightness_default;
	struct sensor_regset preview;
	struct sensor_regset stop_stream;
	struct sensor_regset resol_640_480;
};

#define SENSOR_REGISTER_REGSET(y)		\
	{					\
		.reg	= (y),			\
		.size	= ARRAY_SIZE((y)),	\
	}

static struct sensor_reg sr030_Init_Reg[] = {
{0x03, 0x00,},
{0x01, 0x01,},	/*sleep*/
{0x01, 0x03,},	/*s/w reset*/
{0x01, 0x01,},	/*sleep*/
{0x08, 0x00,}, /*Don't touch*/
{0x09, 0x37,}, /*Don't touch*/
{0x0a, 0x33,}, /*Don't touch*/
{0xd0, 0x05,},
{0xd1, 0x30,},
{0xd2, 0x01,},
{0xd3, 0x20,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x95,},
{0x10, 0x11,},
{0x11, 0x90,},	/*xy flip*/
{0x12, 0x00,},
{0x14, 0x88,},
{0x03, 0x00,},
{0x20, 0x00,},
{0x21, 0x04,},
{0x22, 0x00,},
{0x23, 0x04,},
{0x24, 0x03,},
{0x25, 0xC0,},
{0x26, 0x05,},
{0x27, 0x00,},
{0x40, 0x01,}, /*Hblank_280*/
{0x41, 0x18,},
{0x42, 0x00,}, /*Vblank 124*/
{0x43, 0x7c,},
{0x80, 0x08,},  /*Don't touch */
{0x81, 0x97,},  /*Don't touch */
{0x82, 0x90,},  /*Don't touch */
{0x83, 0x30,},  /*Don't touch */
{0x84, 0xcc,}, /*Don't touch*/
{0x85, 0x00,},  /*Don't touch*/
{0x86, 0xd4,},  /*Don' t touch*/
{0x87, 0x0f,},   /*Don't touch*/
{0x88, 0x34,},  /*Don't touch*/
{0x90, 0x0f,}, /*BLC_TIME_TH_ON*/
{0x91, 0x0f,}, /*BLC_TIME_TH_OFF */
{0x92, 0xf0,}, /*BLC_AG_TH_ON*/
{0x93, 0xe8,}, /*BLC_AG_TH_OFF*/
{0x94, 0x95,}, /*091202*/
{0x95, 0x90,}, /*091202 */
{0x98, 0x38,},  /*Don't touch*/
{0xa0, 0x01,},  /* 20100309*/
{0xa2, 0x01,},  /* 20100309*/
{0xa4, 0x01,},  /* 20100309*/
{0xa6, 0x01,},  /* 20100309*/
{0xa8, 0x00,},
{0xaa, 0x00,},
{0xac, 0x00,},
{0xae, 0x00,},
{0x99, 0x00,},
{0x9a, 0x00,},
{0x9b, 0x00,},
{0x9c, 0x00,},
{0x03, 0x02,},
{0x12, 0x00,}, /*Don't touch*/
{0x14, 0x00,}, /*Don't touch*/
{0x15, 0x00,}, /*Don't touch*/
{0x18, 0x4C,}, /*Don't touch*/
{0x19, 0x00,}, /*Don't touch*/
{0x1A, 0x39,}, /*Don't touch*/
{0x1B, 0x00,}, /*Don't touch*/
{0x1C, 0x1a,}, /*Don't touch*/
{0x1D, 0x14,}, /*Don't touch*/
{0x1E, 0x30,}, /*Don't touch*/
{0x1F, 0x10,}, /*Don't touch*/
{0x20, 0x77,},
{0x21, 0xde,},
{0x22, 0xa7,},
{0x23, 0x30,},
{0x24, 0x77,},
{0x25, 0x10,},
{0x26, 0x10,},
{0x27, 0x3c,},
{0x2b, 0x80,},
{0x2c, 0x02,},
{0x2d, 0xa0,},
{0x2e, 0x00,},
{0x2f, 0xa7,},
{0x30, 0x00,},
{0x31, 0x99,},
{0x32, 0x00,},
{0x33, 0x00,},
{0x34, 0x22,},
{0x36, 0x01,},
{0x37, 0x01,},
{0x38, 0x88,},
{0x39, 0x88,},
{0x3d, 0x03,},
{0x3e, 0x0d,},
{0x3f, 0x02,},
{0x49, 0xd1,},
{0x4a, 0x14,},
{0x50, 0x21,},
{0x52, 0x01,},
{0x53, 0x81,},
{0x54, 0x10,},
{0x55, 0x1c,},
{0x56, 0x11,},
{0x58, 0x18,},
{0x59, 0x16,},
{0x5d, 0xa2,},
{0x5e, 0x5a,},
{0x60, 0x93,},	/* 20120517 modify*/
{0x61, 0xa4,},	/* 20120517 modify*/
{0x62, 0x94,},	/* 20120517 modify*/
{0x63, 0xa3,},   /* 20120517 modify*/
{0x64, 0x94,},	/* 20120517 modify*/
{0x65, 0xa3,},   /* 20120517 modify*/
{0x67, 0x0c,},
{0x68, 0x0c,},
{0x69, 0x0c,},
{0x6a, 0xb4,},
{0x6b, 0xc4,},
{0x6c, 0xb5,},
{0x6d, 0xc2,},
{0x6e, 0xb5,},
{0x6f, 0xc0,},
{0x70, 0xb6,},
{0x71, 0xb8,},
{0x72, 0x95,},	/* 20120517 modify*/
{0x73, 0xa2,},	/* 20120517 modify*/
{0x74, 0x95,},	/* 20120517 modify*/
{0x75, 0xa2,},	/* 20120517 modify*/
{0x76, 0x95,},	/* 20120517 modify*/
{0x77, 0xa2,},	/* 20120517 modify*/
{0x7C, 0x92,},	/* 20120517 modify*/
{0x7D, 0xff,},	/* 20120517 modify*/
{0x80, 0x01,},	/* 20120517 modify*/
{0x81, 0x8a,},   /* 20120517 modify*/
{0x82, 0x1e,},	/* 20120517 modify*/
{0x83, 0x36,},	/* 20120517 modify*/
{0x84, 0x89,},	/* 20120517 modify*/
{0x85, 0x8b,},	/* 20120517 modify*/
{0x86, 0x89,},	/* 20120517 modify*/
{0x87, 0x8b,},	/* 20120517 modify*/
{0x88, 0xab,},
{0x89, 0xbc,},
{0x8a, 0xac,},
{0x8b, 0xba,},
{0x8c, 0xad,},
{0x8d, 0xb8,},
{0x8e, 0xae,},
{0x8f, 0xb2,},
{0x90, 0xb3,},
{0x91, 0xb7,},
{0x92, 0x52,},	/* 20120517 modify*/
{0x93, 0x6a,},	/* 20120517 modify*/
{0x94, 0x89,},	/* 20120517 modify*/
{0x95, 0x8b,},	/* 20120517 modify*/
{0x96, 0x89,},	/* 20120517 modify*/
{0x97, 0x8b,},	/* 20120517 modify*/
{0xA0, 0x02,},
{0xA1, 0x86,},	/* 20120517 modify*/
{0xA2, 0x02,},
{0xA3, 0x86,},	/* 20120517 modify*/
{0xA4, 0x86,},	/* 20120517 modify*/
{0xA5, 0x02,},
{0xA6, 0x86,},	/* 20120517 modify*/
{0xA7, 0x02,},
{0xA8, 0x92,},	/* 20120517 modify*/
{0xA9, 0x94,},	/* 20120517 modify*/
{0xAA, 0x92,},	/* 20120517 modify*/
{0xAB, 0x94,},	/* 20120517 modify*/
{0xAC, 0x1c,},
{0xAD, 0x22,},
{0xAE, 0x1c,},
{0xAF, 0x22,},
{0xB0, 0xa4,},	/* 20120517 modify*/
{0xB1, 0xae,},	/* 20120517 modify*/
{0xB2, 0xa4,},	/* 20120517 modify*/
{0xB3, 0xae,},	/* 20120517 modify*/
{0xB4, 0xa6,},	/* 20120517 modify*/
{0xB5, 0xac,},	/* 20120517 modify*/
{0xB6, 0xa6,},	/* 20120517 modify*/
{0xB7, 0xac,},	/* 20120517 modify*/
{0xB8, 0xa6,},	/* 20120517 modify*/
{0xB9, 0xab,},	/* 20120517 modify*/
{0xBA, 0xa6,},	/* 20120517 modify*/
{0xBB, 0xab,},	/* 20120517 modify*/
{0xBC, 0xa6,},	/* 20120517 modify*/
{0xBD, 0xab,},	/* 20120517 modify*/
{0xBE, 0xa6,},	/* 20120517 modify*/
{0xBF, 0xab,},	/* 20120517 modify*/
{0xc4, 0x37,},
{0xc5, 0x52,},
{0xc6, 0x6b,},
{0xc7, 0x86,},
{0xc8, 0x38,},	/* 20120517 modify*/
{0xc9, 0x50,},	/* 20120517 modify*/
{0xca, 0x38,},	/* 20120517 modify*/
{0xcb, 0x50,},	/* 20120517 modify*/
{0xcc, 0x6c,},	/* 20120517 modify*/
{0xcd, 0x84,},	/* 20120517 modify*/
{0xce, 0x6c,},	/* 20120517 modify*/
{0xcf, 0x84,},	/* 20120517 modify*/
{0xdc, 0x00,},   /* Added*/
{0xdd, 0xaf,},   /* Added*/
{0xde, 0x00,},   /* Added*/
{0xdf, 0x90,},   /* Added*/
{0xd0, 0x10,},
{0xd1, 0x14,},
{0xd2, 0x20,},
{0xd3, 0x00,},
{0xd4, 0x0c,}, /*DCDC_TIME_TH_ON*/
{0xd5, 0x0c,}, /*DCDC_TIME_TH_OFF */
{0xd6, 0xf0,}, /*DCDC_AG_TH_ON*/
{0xd7, 0xe8,}, /*DCDC_AG_TH_OFF*/
{0xea, 0x8a,},
{0xF0, 0x01,},	/* clock inversion*/
{0xF1, 0x01,},
{0xF2, 0x01,},
{0xF3, 0x01,},
{0xF4, 0x01,},
{0xF5, 0x00,},
{0x03, 0x10,},	/*page 10*/
{0x10, 0x01,},	/*Ycbcr422_bit Order: YUYV*/
{0x11, 0x03,},
{0x12, 0x30,},	/*y offset[4], dif_offset[5]*/
{0x13, 0x02,},	/*contrast effet enable : 0x02*/
{0x34, 0x00,},	/*hidden 10->00 100209*/
{0x37, 0x01,},	/*yc2d power save */
{0x3f, 0x04,},	/*100825*/
{0x40, 0x80,},	/*Y offset  */
{0x48, 0x80,},
{0x53, 0x00,},	/*dif_offset option */
{0x55, 0x30,},	/*dif_offset option  diff_offset max */
{0x60, 0x4f,},	/*out color sat en[7] | auto color decrement en[1] /	| manual color sat en[0]*/
{0x61, 0x83,},	/*blue saturation_C0*/
{0x62, 0x80,},	/*red saturation_B0*/
{0x63, 0xff,},	/*auto decresment on AG th*/
{0x64, 0xc0,},	/*auto decresment on DG th*/
{0x66, 0xe4,},	/*Outdoor saturation step 137fps apply out th */
{0x67, 0x03,}, /*OutdoorsaturationB/R*/
{0x76, 0x01,}, /* ADD 20121031 */
{0x79, 0x04,}, /* ADD 20121031 */
{0x03, 0x10,},
{0x80, 0x00,}, /* dsshin --> color enhance*/
{0xf5, 0x00,}, /* dsshin --> h blank option*/
{0x03, 0x11,},	/*page 11 D_LPF	*/
{0x10, 0x3f,},	/*B[6]:Blue En  Dlpf on[4:0] Sky over off : 0x7f->3f*/
{0x11, 0x20,},	/* Uniform Full GbGr/OV-Nr*/
{0x12, 0x80,},	/*Blue MaxOpt  blue sky max filter optoin rate : 0 0xc0->80*/
{0x13, 0xb8,},	/*dark2[7] | ratio[6:4] | dark3[3] | dark3 maxfilter ratio[2:0] */
{0x30, 0xba,},	/*Outdoor2 H th*/
{0x31, 0x10,},	/*Outdoor2 L th*/
{0x32, 0x50,},	/*Outdoor2 gain ratio*/
{0x33, 0x1d,},	/*Outdoor2 H lum*/
{0x34, 0x20,},	/*Outdoor2 M lum*/
{0x35, 0x1f,},	/*Outdoor2 L lum*/
{0x36, 0xb0,},	/*Outdoor1 H th*/
{0x37, 0x18,},	/*Outdoor1 L th*/
{0x38, 0x50,},	/*Outdoor1 gain ratio  0x80->40*/
{0x39, 0x1d,},	/*Outdoor1 H lum       0x28->1e*/
{0x3a, 0x20,},	/*Outdoor1 M lum       0x10->15*/
{0x3b, 0x1f,},	/*Outdoor1 L lum       0x08->20*/
{0x3c, 0x3f,},	/*indoor H th*/
{0x3d, 0x16,},	/*indoor L th*/
{0x3e, 0x30,},	/*indoor gain ratio    0x44  6a */
{0x3f, 0x1a,},	/*indoor H lum         0x12  18	*/
{0x40, 0x60,},	/*indoor M lum	       0x18  1c*/
{0x41, 0x1a,},	/*indoor L lum         0x18  3e*/
{0x42, 0x98,},	/*dark1 H th*/
{0x43, 0x28,},	/*dark1 L th*/
{0x44, 0x65,},	/*dark1 gain ratio*/
{0x45, 0x16,},	/*dark1 H lum         0x38->0x28  */
{0x46, 0x30,},	/*dark1 M lum         0x27->0x17*/
{0x47, 0x34,},	/*dark1 L lum         0x20->0x1a */
{0x48, 0x90,},	/*dark2 H th*/
{0x49, 0x2a,},	/*dark2 L th*/
{0x4a, 0x65,},	/*dark2 gain ratio*/
{0x4b, 0x18,},	/*dark2 H lum */
{0x4c, 0x31,},	/*dark2 M lum*/
{0x4d, 0x36,},	/*dark2 L lum */
{0x4e, 0x80,},	/*dark3 H th*/
{0x4f, 0x30,},	/*dark3 L th*/
{0x50, 0x65,},	/*dark3 gain ratio*/
{0x51, 0x19,},	/*dark3 H lum	*/
{0x52, 0x31,},	/*dark3 M lum */
{0x53, 0x36,},	/*dark3 L lum */
{0x5a, 0x3f,},	/*blue sky mode out1/2 enable  0x27->3f */
{0x5b, 0x00,},	/*Impulse pixel enable dark123,in,out123 ::  must be 0x07*/
{0x5c, 0x9f,},	/*Indoor maxfilter rate[7:5] | Uncertain onoff[4:0] 0x1f ->0x9f*/
{0x60, 0x3f,},	/*GbGr all enable*/
{0x62, 0x0f,},	/*GbGr offset*/
{0x65, 0x0c,},	/*Outdoor GbGr rate H 100% M 25% L 100%*/
{0x66, 0x0c,},	/*Indoor GbGr  rate H 100% M 25% L 100%*/
{0x67, 0x00,},	/*dark GbGr    rate H/M/L  100%*/
{0x70, 0x0c,},	/* Abberation On/Off B[1]: Outdoor B[0]: Indoor 07>>c*/
{0x75, 0xa0,},	/* Outdoor2 Abberation Luminance lvl */
{0x7d, 0xb4,},	/* Indoor Abberation Luminance   lvl*/
{0x96, 0x08,},	/*indoor/Dark1 edgeoffset1*/
{0x97, 0x14,},	/*indoor/Dark1 center G value*/
{0x98, 0xf5,},	/*slope indoor :: left/right graph polarity, slope*/
{0x99, 0x2a,},	/*indoor uncertain ratio control*/
{0x9a, 0x20,},	/*Edgeoffset_dark*/
{0x03, 0x12,},	/*Preview DPC off[0x5c] on[0x5d]*/
{0x20, 0x0f,},
{0x21, 0x0f,},
{0x25, 0x00,}, /* 0x30*/
{0x2a, 0x01,},
{0x2e, 0x00,},	/*2010.8.25*/
{0x30, 0x35,},	/*Texture region(most detail)*/
{0x31, 0xa0,},	/*STD uniform1 most blur region*/
{0x32, 0xb0,},	/*STD uniform2      2nd blur*/
{0x33, 0xc0,},	/*STD uniform3      3rd blur*/
{0x34, 0xd0,},	/*STD normal noise1 4th blur	*/
{0x35, 0xe0,},	/*STD normal noise2 5th blur*/
{0x36, 0xff,},	/*STD normal noise3 6th blur*/
{0x40, 0x83,},	/*Outdoor2 H th*/
{0x41, 0x20,},	/*Outdoor2 L th		*/
{0x42, 0x08,},	/*Outdoor2 H luminance */
{0x43, 0x10,},	/*Outdoor2 M luminance */
{0x44, 0x10,},	/*Outdoor2 l luminance */
{0x45, 0x50,},	/*Outdoor2 ratio*/
{0x46, 0x83,},	/*Outdoor1 H th*/
{0x47, 0x20,},	/*Outdoor1 L th		*/
{0x48, 0x08,},	/*Outdoor1 H luminance*/
{0x49, 0x10,},	/*Outdoor1 M luminance*/
{0x4a, 0x10,},	/*Outdoor1 L luminance*/
{0x4b, 0x50,},	/*Outdoor1 ratio*/
{0x4c, 0x80,},	/*Indoor H th*/
{0x4d, 0x48,},	/*Indoor L th*/
{0x4e, 0x30,},	/*indoor H lum*/
{0x4f, 0x30,},	/*indoor M lum*/
{0x50, 0x12,},	/*indoor L lum */
{0x51, 0x70,},	/*indoor ratio  0x10 -> 0x45*/
{0x52, 0xa8,},	/*dark1 H th*/
{0x53, 0x30,},	/*dark1 L th	*/
{0x54, 0x28,},	/*dark1 H lum */
{0x55, 0x3e,},	/*dark1 M lum*/
{0x56, 0x67,},	/*dark1 L lum*/
{0x57, 0x6a,},	/*dark1 ratio*/
{0x58, 0xa0,},	/*dark2 H th*/
{0x59, 0x40,},	/*dark2 L th*/
{0x5a, 0x28,},	/*dark2 H lum*/
{0x5b, 0x3f,},	/*dark2 M lum*/
{0x5c, 0x68,},	/*dark2 L lum*/
{0x5d, 0x70,},	/*dark2 ratio*/
{0x5e, 0xa0,},	/*dark3 H th*/
{0x5f, 0x40,},	/*dark3 L th*/
{0x60, 0x29,},	/*dark3 H lum*/
{0x61, 0x3f,},	/*dark3 M lum*/
{0x62, 0x69,},	/*dark3 L lum*/
{0x63, 0x6a,},	/*dark3 ratio*/
{0x70, 0x10,},
{0x71, 0x0a,},
{0x72, 0x10,},
{0x73, 0x0a,},
{0x74, 0x18,},
{0x75, 0x12,},
{0x80, 0x20,},
{0x81, 0x40,},
{0x82, 0x65,},
{0x85, 0x1a,},
{0x88, 0x00,},
{0x89, 0x00,},
{0x90, 0x5d,},	/*Preview DPC off[0x5c] on[0x5d]*/
{0xad, 0x07,},	/*10825*/
{0xae, 0x07,},	/*10825*/
{0xaf, 0x07,},	/*10825*/
{0xc5, 0x58,},	/*BlueRange   2010.8.25    0x40->23 */
{0xc6, 0x20,},	/*GreenRange  2010.8.25    0x3b->20 */
{0xd0, 0x88,},	/*2010.8.25*/
{0xd1, 0x80,},
{0xd2, 0x17,}, /*preview 17, full 67*/
{0xd3, 0x00,},
{0xd4, 0x00,},
{0xd5, 0x0f,}, /*preview 0f, full 02*/
{0xd6, 0xff,},
{0xd7, 0xff,}, /*preview ff, full 18*/
{0xd8, 0x00,},
{0xd9, 0x04,},
{0xdb, 0x38,},	/*resolution issue 0x00->0x18->0x38 */
{0xd9, 0x04,},	/*strong_edge detect ratio*/
{0xe0, 0x01,},	/*strong_edge detect ratio*/
{0x03, 0x13,},	/*page 13 sharpness 1D*/
{0x10, 0xc5,},
{0x11, 0x7b,},
{0x12, 0x0e,},
{0x14, 0x00,},
{0x15, 0x11,},	/*added option 1.3M*/
{0x18, 0x30,},	/*added option 1.3M*/
{0x20, 0x15,},
{0x21, 0x13,},
{0x22, 0x33,},
{0x23, 0x08,},	/*hi_clip th1*/
{0x24, 0x1a,},	/*hi_clip th2*/
{0x25, 0x06,},	/*low clip th*/
{0x26, 0x18,},
{0x27, 0x30,},
{0x29, 0x10,},	/*time th*/
{0x2a, 0x30,},	/*pga th*/
{0x2b, 0x03,},	/*lpf out2*/
{0x2c, 0x03,},	/*lpf out1*/
{0x2d, 0x0c,},
{0x2e, 0x12,},
{0x2f, 0x12,},
{0x50, 0x0a,},	/*out2  hi nega*/
{0x53, 0x07,},	/*      hi pos*/
{0x51, 0x0c,},	/*      mi nega*/
{0x54, 0x07,},	/*      mi pos*/
{0x52, 0x0b,},	/*      lo nega*/
{0x55, 0x08,},	/*      lo pos*/
{0x56, 0x0a,},	/*out1   hi nega*/
{0x59, 0x07,},	/*       hi pos */
{0x57, 0x0c,},	/*       mi nega*/
{0x5a, 0x07,},	/*       mi pos */
{0x58, 0x0b,},	/*       lo nega*/
{0x5b, 0x08,},	/*       lo pos	*/
{0x5c, 0x08,},  /*indoor hi  nega*/
{0x5f, 0x07,},  /*       hi  pos*/
{0x5d, 0x14,},
{0x60, 0x12,},
{0x5e, 0x0a,},
{0x61, 0x08,},  /*       low pos*/
{0x62, 0x08,},  /*dark1  hi nega*/
{0x65, 0x06,},  /*       hi  pos   */
{0x63, 0x08,},  /*       mid nega  */
{0x66, 0x06,},  /*       mid pos   */
{0x64, 0x08,},  /*       low nega  */
{0x67, 0x06,},  /*       low pos   */
{0x68, 0x07,},  /*dark2  hi  nega*/
{0x6b, 0x05,},  /*       hi  pos       */
{0x69, 0x07,},  /*       mid nega      */
{0x6c, 0x05,},  /*       mid pos       */
{0x6a, 0x07,},  /*       low nega      */
{0x6d, 0x05,},  /*       low pos       */
{0x6e, 0x0a,},  /*dark3  hi  nega*/
{0x71, 0x09,},  /*       hi  pos       */
{0x6f, 0x0a,},  /*       mid nega      */
{0x72, 0x09,},  /*       mid pos       */
{0x70, 0x0a,},  /*       low nega      */
{0x73, 0x09,},  /*       low pos       */
{0x80, 0xc1,},
{0x81, 0x1f,},
{0x82, 0xe1,},
{0x83, 0x33,},
{0x90, 0x05,},
{0x91, 0x05,},
{0x92, 0x33,},
{0x93, 0x30,},
{0x94, 0x03,},
{0x95, 0x14,},
{0x97, 0x30,},
{0x99, 0x30,},
{0xa0, 0x02,},	/*2d lclp out2  nega*/
{0xa1, 0x03,},	/*2d lclp out2  pos*/
{0xa2, 0x02,},	/*2d lclp out1  nega*/
{0xa3, 0x03,},	/*2d lclp out1  pos*/
{0xa4, 0x03,},	/*2d lclp in    nega*/
{0xa5, 0x04,},	/*2d lclp in    pos*/
{0xa6, 0x07,},	/*2d lclp dark1 nega*/
{0xa7, 0x08,},	/*2d lclp dark1 pos*/
{0xa8, 0x07,},	/*2d lclp dark2 nega*/
{0xa9, 0x08,},	/*2d lclp dark2 pos*/
{0xaa, 0x07,},	/*2d lclp dark3 nega*/
{0xab, 0x08,},	/*2d lclp dark3 pos*/
{0xb0, 0x10,},	/*out2   H Ne*/
{0xb3, 0x10,},	/*       H Po*/
{0xb1, 0x1e,},	/*       M Ne*/
{0xb4, 0x1e,},	/*       M Po*/
{0xb2, 0x1f,},	/*       L Ne*/
{0xb5, 0x1e,},	/*       L Po*/
{0xb6, 0x10,},	/*out1   H Ne   */
{0xb9, 0x10,},	/*       H Po   */
{0xb7, 0x1e,},	/*       M Ne   */
{0xba, 0x1e,},	/*       M Po   */
{0xb8, 0x1f,},	/*       L Ne   */
{0xbb, 0x1e,},	/*       L Po   */
{0xbc, 0x20,},  /*indoor H Ne*/
{0xbf, 0x1e,},  /*       H Po*/
{0xbd, 0x25,},  /*       M Ne*/
{0xc0, 0x23,},  /*       M Po*/
{0xbe, 0x24,},  /*       L Ne*/
{0xc1, 0x22,},  /*       L Po*/
{0xc2, 0x23,},  /*dark1  H Ne*/
{0xc5, 0x23,},  /*       H Po*/
{0xc3, 0x29,},  /*       M Ne*/
{0xc6, 0x29,},  /*       M Po*/
{0xc4, 0x25,},  /*       L Ne*/
{0xc7, 0x25,},  /*       L Po*/
{0xc8, 0x1c,},  /*dark2   H Ne*/
{0xcb, 0x1c,},  /*       H Po*/
{0xc9, 0x25,},  /*       M Ne*/
{0xcc, 0x25,},  /*       M Po*/
{0xca, 0x23,},  /*       L Ne*/
{0xcd, 0x23,},  /*       L Po*/
{0xce, 0x1c,},  /*dark3  H Ne*/
{0xd1, 0x1c,},  /*       H Po*/
{0xcf, 0x25,},  /*       M Ne*/
{0xd2, 0x25,},  /*       M Po*/
{0xd0, 0x23,},  /*       L Ne*/
{0xd3, 0x23,},  /*       L Po*/
{0x03, 0x14,},
{0x10, 0x31,},
{0x14, 0x80,}, /* GX*/
{0x15, 0x80,}, /* GY*/
{0x16, 0x80,}, /* RX*/
{0x17, 0x80,}, /* RY*/
{0x18, 0x80,}, /* BX*/
{0x19, 0x80,}, /* BY*/
{0x20, 0x60,}, /* X Center*/
{0x21, 0x80,}, /* Y Center*/
{0x22, 0x80,},
{0x23, 0x80,},
{0x24, 0x80,},
{0x30, 0xc8,},
{0x31, 0x2b,},
{0x32, 0x00,},
{0x33, 0x00,},
{0x34, 0x90,},
{0x40, 0x56,}, /*Rmin'sset4e*/
{0x41, 0x3a,}, /*Gr*/
{0x42, 0x37,}, /*B*/
{0x43, 0x3a,}, /*Gb*/
{0x03, 0x15,},
{0x10, 0x21,},
{0x14, 0x44,}, /*49*/
{0x15, 0x34,}, /*38*/
{0x16, 0x26,}, /*2b*/
{0x17, 0x2f,},
{0x30, 0xdd,},
{0x31, 0x62,},
{0x32, 0x05,},
{0x33, 0x26,},
{0x34, 0xbd,},
{0x35, 0x17,},
{0x36, 0x18,},
{0x37, 0x38,},
{0x38, 0xd0,},
{0x40, 0xb0,},
{0x41, 0x30,},
{0x42, 0x00,},
{0x43, 0x00,},
{0x44, 0x00,},
{0x45, 0x00,},
{0x46, 0x99,},
{0x47, 0x19,},
{0x48, 0x00,},
{0x50, 0x16,},
{0x51, 0xb2,},
{0x52, 0x1c,},
{0x53, 0x06,},
{0x54, 0x20,},
{0x55, 0xa6,},
{0x56, 0x0e,},
{0x57, 0xb2,},
{0x58, 0x24,},
{0x03, 0x16,},
{0x10, 0x31,},	/*GMA_CTL*/
{0x18, 0x7e,},	/*AG_ON*/
{0x19, 0x7d,},	/*AG_OFF*/
{0x1a, 0x0e,},	/*TIME_ON*/
{0x1b, 0x01,},	/*TIME_OFF*/
{0x1C, 0xdc,},	/*OUT_ON*/
{0x1D, 0xfe,},	/*OUT_OFF*/
{0x30, 0x00,},
{0x31, 0x07,},
{0x32, 0x1a,},
{0x33, 0x35,},
{0x34, 0x5a,},
{0x35, 0x7c,},
{0x36, 0x96,},
{0x37, 0xa9,},
{0x38, 0xb7,},
{0x39, 0xc6,},
{0x3a, 0xd2,},
{0x3b, 0xdc,},
{0x3c, 0xe4,},
{0x3d, 0xeb,},
{0x3e, 0xf1,},
{0x3f, 0xf5,},
{0x40, 0xf9,},
{0x41, 0xfd,},
{0x42, 0xff,},
{0x50, 0x00,},
{0x51, 0x03,},
{0x52, 0x13,},
{0x53, 0x2e,},
{0x54, 0x59,},
{0x55, 0x79,},
{0x56, 0x90,},
{0x57, 0xa3,},
{0x58, 0xb4,},
{0x59, 0xc2,},
{0x5a, 0xcd,},
{0x5b, 0xd7,},
{0x5c, 0xe0,},
{0x5d, 0xe5,},
{0x5e, 0xe9,},
{0x5f, 0xee,},
{0x60, 0xf1,},
{0x61, 0xf3,},
{0x62, 0xf6,},
{0x70, 0x03,},
{0x71, 0x11,},
{0x72, 0x1f,},
{0x73, 0x37,},
{0x74, 0x52,},
{0x75, 0x6c,},
{0x76, 0x85,},
{0x77, 0x9a,},
{0x78, 0xad,},
{0x79, 0xbd,},
{0x7a, 0xcb,},
{0x7b, 0xd6,},
{0x7c, 0xe0,},
{0x7d, 0xe8,},
{0x7e, 0xef,},
{0x7f, 0xf4,},
{0x80, 0xf8,},
{0x81, 0xfb,},
{0x82, 0xfe,},
{0x03, 0x24,},	/*Resol control */
{0x60, 0xc5,},	/*edge even frame | 16bit resol | white edge cnt | scene resol enable*/
{0x61, 0x04,},	/*even frame update	*/
{0x64, 0x08,},
{0x65, 0x00,},
{0x66, 0x26,},	/*edge th2 H	*/
{0x67, 0x00,},	/*edge th2 L	*/
{0x03, 0x13,},
{0x18, 0x31,},	/*flat center Gb/Gr*/
{0x74, 0x02,},	/*det slope en | gausian filter*/
{0x75, 0x0d,},	/*1D negative gain det 09	*/
{0x76, 0x0d,},	/*1D postive  gain det	08*/
{0x77, 0x10,},	/*1D hclp2 det*/
{0x78, 0x08,},	/*outdoor flat threshold*/
{0x79, 0x10,},	/*indoor flat threshold*/
{0x81, 0xdf,},	/*det gain controler*/
{0x86, 0x90,},	/*2D negative gain det	*/
{0x87, 0x90,},	/*2D postive  gain det	*/
{0x96, 0x2a,},	/*2D hclp2 det*/
{0x03, 0x12,},	/*0x12 page*/
{0xd0, 0x88,},
{0xd9, 0xe4,},
{0x03, 0x18,},
{0x14, 0x43,}, /*83*/
{0x03, 0x20,},
{0x11, 0x1c,},
{0x18, 0x30,},
{0x1a, 0x08,},
{0x20, 0x45,},/*weight*/
{0x21, 0x30,},
{0x22, 0x10,},
{0x23, 0x00,},
{0x24, 0x00,},
{0x28, 0xe7,}, /* add 20120223*/
{0x29, 0x0d,}, /* 20100305 ad -> 0d*/
{0x2a, 0xfd,},
{0x2b, 0xf8,},
{0x2c, 0xc3,},
{0x2d, 0x5f,}, /* add 20120223*/
{0x2e, 0x33,},
{0x30, 0xf8,},
{0x32, 0x03,},
{0x33, 0x2e,},
{0x34, 0x30,},
{0x35, 0xd4,},
{0x36, 0xff,}, /*fe*/
{0x37, 0x32,},
{0x38, 0x04,},
{0x39, 0x22,},
{0x3a, 0xde,},
{0x3b, 0x22,},
{0x3c, 0xde,},
{0x3d, 0xe1,},
{0x50, 0x45,},
{0x51, 0x88,},
{0x56, 0x1a,},
{0x57, 0x80,},
{0x58, 0x0e,},
{0x59, 0x6a,},
{0x5a, 0x04,},
{0x5e, 0x9d,}, /*AE_AWB_start*/
{0x5f, 0x76,}, /*AE_AWB_start*/
{0x70, 0x33,}, /* 6c*/
{0x71, 0x82,}, /* 82(+8)*/
{0x76, 0x21,},
{0x77, 0x71,},
{0x78, 0x22,}, /* 24*/
{0x79, 0x23,}, /* Y Target 70 => 25, 72 => 26*/
{0x7a, 0x23,}, /* 23*/
{0x7b, 0x22,}, /* 22*/
{0x7d, 0x23,},
{0x83, 0x01,}, /*EXP Normal 30.00 fps */
{0x84, 0x86,},
{0x85, 0xa0,},
{0x86, 0x01,}, /*EXPMin 7500.00 fps*/
{0x87, 0x90,},
{0x88, 0x05,}, /*EXP Max(120Hz) 8.00 fps */
{0x89, 0xb8,},
{0x8a, 0xd8,},
{0xa5, 0x05,}, /*EXP Max(100Hz) 8.33 fps */
{0xa6, 0x7e,},
{0xa7, 0x40,},
{0x8B, 0x75,}, /*EXP100 */
{0x8C, 0x30,},
{0x8D, 0x61,}, /*EXP120 */
{0x8E, 0xa8,},
{0x9c, 0x09,}, /*EXP Limit 1250.00 fps */
{0x9d, 0x60,},
{0x9e, 0x01,}, /*EXP Unit */
{0x9f, 0x90,},
{0x98, 0x9d,},
{0xb0, 0x16,},
{0xb1, 0x14,},
{0xb2, 0xf8,},
{0xb3, 0x14,},
{0xb4, 0x1b,},
{0xb5, 0x46,},
{0xb6, 0x31,},
{0xb7, 0x29,},
{0xb8, 0x26,},
{0xb9, 0x24,},
{0xba, 0x22,},
{0xbb, 0x42,},
{0xbc, 0x41,},
{0xbd, 0x40,},
{0xc0, 0x10,},
{0xc1, 0x38,},
{0xc2, 0x38,},
{0xc3, 0x38,},
{0xc4, 0x07,},
{0xc8, 0x80,},
{0xc9, 0x80,},
{0x10, 0x8c,},	/* ae enable*/
{0x03, 0x21,},
{0x20, 0x11,},
{0x21, 0x11,},
{0x22, 0x11,},
{0x23, 0x11,},
{0x24, 0x12,},
{0x25, 0x22,},
{0x26, 0x22,},
{0x27, 0x21,},
{0x28, 0x12,},
{0x29, 0x22,},
{0x2a, 0x22,},
{0x2b, 0x21,},
{0x2c, 0x12,},
{0x2d, 0x23,},
{0x2e, 0x32,},
{0x2f, 0x21,},
{0x30, 0x12,},
{0x31, 0x23,},
{0x32, 0x32,},
{0x33, 0x21,},
{0x34, 0x12,},
{0x35, 0x22,},
{0x36, 0x22,},
{0x37, 0x21,},
{0x38, 0x12,},
{0x39, 0x22,},
{0x3a, 0x22,},
{0x3b, 0x21,},
{0x3c, 0x11,},
{0x3d, 0x11,},
{0x3e, 0x11,},
{0x3f, 0x11,},
{0x03, 0x22,},
{0x10, 0xfd,},
{0x11, 0x2e,},
{0x19, 0x01,}, /* Low On*/
{0x20, 0x30,}, /* for wb speed*/
{0x21, 0x40,},
{0x24, 0x01,},
{0x25, 0x7e,}, /* for tracking 20120314 */
{0x30, 0x80,}, /* 20120224 test*/
{0x31, 0x80,},
{0x38, 0x11,},
{0x39, 0x34,},
{0x40, 0xe8,},
{0x41, 0x43,}, /* 33*/
{0x42, 0x22,}, /* 22*/
{0x43, 0xf3,}, /* f6*/
{0x44, 0x54,}, /* 44*/
{0x45, 0x22,}, /* 33*/
{0x46, 0x00,},
{0x48, 0x0a,},
{0x50, 0xb2,},
{0x51, 0x81,},
{0x52, 0x98,},
{0x80, 0x38,},
{0x81, 0x20,},
{0x82, 0x38,}, /* 3a*/
{0x83, 0x56,}, /* R Max*/
{0x84, 0x20,}, /* R Min*/
{0x85, 0x52,}, /* B Max*/
{0x86, 0x20,}, /* B Min*/
{0x87, 0x46,},
{0x88, 0x36,},
{0x89, 0x3a,},
{0x8a, 0x2f,},
{0x8b, 0x3d,},
{0x8c, 0x37,},
{0x8d, 0x35,},
{0x8e, 0x32,},
{0x8f, 0x5a,},
{0x90, 0x59,},
{0x91, 0x55,},
{0x92, 0x4e,},
{0x93, 0x44,},
{0x94, 0x3a,},
{0x95, 0x34,},
{0x96, 0x2c,},
{0x97, 0x23,},
{0x98, 0x20,},
{0x99, 0x1f,},
{0x9a, 0x1f,},
{0x9b, 0x77,},
{0x9c, 0x77,},
{0x9d, 0x48,},
{0x9e, 0x38,},
{0x9f, 0x30,},
{0xa0, 0x40,},
{0xa1, 0x21,},
{0xa2, 0x6f,},
{0xa3, 0xff,},
{0xa4, 0x14,}, /* 1500fps*/
{0xa5, 0x44,}, /* 700fps*/
{0xa6, 0xcf,},
{0xad, 0x40,},
{0xae, 0x4a,},
{0xaf, 0x2a,},  /* low temp Rgain*/
{0xb0, 0x28,},  /* low temp Rgain*/
{0xb1, 0x00,}, /* 0x20 -> 0x00 0405 modify*/
{0xb4, 0xbf,}, /* for tracking 20120314*/
{0xb8, 0xa1,}, /*a2:b-2,R+2b4B-3,R+4lowtempb0a1SpecAWBAmodify*/
{0xb9, 0x00,},
{0x03, 0x00,},
{0xd0, 0x05,},
{0xd1, 0x30,},
{0xd2, 0x05,},
{0xd3, 0x20,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x95,},
{0x03, 0x48,},
{0x10, 0x1c,},
{0x11, 0x00,},
{0x12, 0x00,},
{0x14, 0x00,},
{0x16, 0x04,},
{0x17, 0x00,},
{0x18, 0x80,},
{0x19, 0x00,},
{0x1a, 0xa0,},
{0x1c, 0x02,},
{0x1d, 0x0e,},
{0x1e, 0x07,},
{0x1f, 0x08,},
{0x22, 0x00,},
{0x23, 0x01,},
{0x24, 0x1e,},
{0x25, 0x00,},
{0x26, 0x00,},
{0x27, 0x08,},
{0x28, 0x00,},
{0x30, 0x05,},
{0x31, 0x00,},
{0x32, 0x07,},
{0x33, 0x09,},
{0x34, 0x01,},
{0x35, 0x01,},
{0x03, 0x00,},
{0x01, 0x01,},
};

static struct sensor_reg sr030_effect_none[] = {
{0x03, 0x10,},
{0x11, 0x03,},
{0x12, 0x30,},
{0x13, 0x02,},
{0x40, 0x80,},
{0x44, 0x80,},
{0x45, 0x80,},
};

static struct sensor_reg  sr030_brightness_default[] = {
{0x03, 0x10,},
{0x40, 0x80,},
};

/* WhiteBalance Mode */
static struct sensor_reg sr030_wb_auto[] = {
{0x03, 0x22,},
{0x10, 0x6b,},
{0x11, 0x2e,},
{0x80, 0x38,},
{0x81, 0x20,},
{0x82, 0x38,},
{0x83, 0x56,},
{0x84, 0x20,},
{0x85, 0x52,},
{0x86, 0x20,},
{0x10, 0xeb,},
};

static struct sensor_reg sr030_resol_640_480[] = {
{0x03, 0x00,},
{0x01, 0x01,},/*sleep*/
{0xd0, 0x05,},/*Pll Off*/
{0x03, 0x20,},
{0x10, 0x0c,},/*AE off (0x0c:60Hz   0x1c:50Hz)*/
{0x03, 0x22,},
{0x10, 0x7d,},/*AWB off*/
{0x03, 0x00,},
{0x10, 0x11,},
{0x03, 0x11,},
{0x5b, 0x00,},/*don't touch*/
{0x03, 0x12,},
{0x20, 0x0f,},
{0x21, 0x0f,},
{0xd2, 0x17,},
{0xd5, 0x0f,},
{0xd7, 0xff,},
{0x03, 0x13,},
{0x10, 0xc4,},
{0x80, 0xc0,},
{0x03, 0x18,},
{0x14, 0x43,}, /*83*/
{0x03, 0x20,},
{0x10, 0x8c,},  /*AE ON (0x8c:60Hz   0x9c:50Hz)*/
{0x03, 0x22,},
{0x10, 0xfd,},  /*AWB ON*/
{0x03, 0x00,},  /*Page 0 PLL on*/
{0xd0, 0x05,},
{0xd1, 0x30,},
{0xd2, 0x05,},
{0xd3, 0x20,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x85,},
{0xd0, 0x95,},
{0x03, 0x48,},
{0x10, 0x1c,},
{0x11, 0x00,},
{0x12, 0x00,},
{0x14, 0x00,},
{0x16, 0x04,},
{0x17, 0x00,},
{0x18, 0x80,},
{0x19, 0x00,},
{0x1a, 0xa0,},
{0x1c, 0x02,},
{0x1d, 0x0e,},
{0x1e, 0x07,},
{0x1f, 0x08,},
{0x22, 0x00,},
{0x23, 0x01,},
{0x24, 0x1e,},
{0x25, 0x00,},
{0x26, 0x00,},
{0x27, 0x08,},
{0x28, 0x00,},
{0x30, 0x05,},
{0x31, 0x00,},
{0x32, 0x07,},
{0x33, 0x09,},
{0x34, 0x01,},
{0x35, 0x01,},
{0x03, 0x00,},
{0x01, 0x00,},
{0xff, 0x32,},
};

static struct sensor_reg  sr030_stop_stream[] = {
{0x03, 0x00,},
{0x01, 0x01,},
{0xff, 0x02,},
};

#if 0


static struct sensor_reg sr030_effect_negative[] = {
{0x03, 0x10,},
{0x11, 0x03,},
{0x12, 0x38,},
{0x03, 0x13,},
{0x10, 0x3b,},
{0x20, 0x02,},
};

static struct sensor_reg sr030_effect_sepia[] = {
{0x03, 0x10,},
{0x11, 0x03,},
{0x12, 0x30,},
{0xff, 0x03,},
{0x44, 0x70,},
{0x45, 0x98,},
{0x12, 0x33,},
{0x03, 0x13,},
{0x10, 0x3b,},
{0x20, 0x02,},
};

static struct sensor_reg sr030_effect_gray[] = {
{0x03, 0x10,},
{0x11, 0x03,},
{0x12, 0x30,},
{0xff, 0x03,},
{0x44, 0x80,},
{0x45, 0x80,},
{0x12, 0x33,},
{0x03, 0x13,},
{0x10, 0x3b,},
{0x20, 0x02,},
};

static struct sensor_reg  sr030pc50_brightness_M4[] = {
{0x03, 0x10,},
{0x40, 0xd0,},
};

static struct sensor_reg  sr030pc50_brightness_M3[] = {
{0x03, 0x10,},
{0x40, 0xB0,},
};


static struct sensor_reg  sr030pc50_brightness_M2[] = {
{0x03, 0x10,},
{0x40, 0xA0,},
};

static struct sensor_reg  sr030pc50_brightness_M1[] = {
{0x03, 0x10,},
{0x40, 0x90,},
};

static struct sensor_reg  sr030pc50_brightness_P1[] = {
{0x03, 0x10,},
{0x40, 0x10,},
};

static struct sensor_reg  sr030pc50_brightness_P2[] = {
{0x03, 0x10,},
{0x40, 0x20,},
};


static struct sensor_reg  sr030pc50_brightness_P3[] = {
{0x03, 0x10,},
{0x40, 0x30,},
};

static struct sensor_reg  sr030pc50_brightness_P4[] = {
{0x03, 0x10,},
{0x40, 0x50,},
};

static struct sensor_reg  sr030pc50_fps_Auto[] = {
{0x03, 0x00,},
{0x01, 0xf1,},
{0x11, 0x90,},
{0x40, 0x00,},
{0x41, 0x90,},
{0x42, 0x00,},
{0x43, 0x80,},
{0x03, 0x20,},
{0x10, 0x0c,},
{0x2a, 0xf0,},
{0x2b, 0x34,},
{0x30, 0x78,},
{0x83, 0x00,},
{0x84, 0xc3,},
{0x85, 0x50,},
{0x88, 0x02,},
{0x89, 0xdc,},
{0x8a, 0x6c,},
{0x10, 0x8c,},
{0x03, 0x00,},
{0x11, 0x90,},
{0x01, 0xf0,},
};


static struct sensor_reg  sr030pc50_fps_auto_normal_regs[] = {
{0x03, 0x00,},
{0x01, 0x71,}, //sleep
{0x03, 0x00,},
{0x11, 0x90,}, //fixed fps disable
{0x42, 0x00,}, //Vblank 104
{0x43, 0x68,},
{0x90, 0x0c,}, //BLC_TIME_TH_ON
{0x91, 0x0c,}, //BLC_TIME_TH_OFF
{0x92, 0x98,}, //BLC_AG_TH_ON
{0x93, 0x90,}, //BLC_AG_TH_OFF
{0x03, 0x20,},
{0x10, 0x1c,}, //ae off
{0x2a, 0xf0,}, //antibanding
{0x2b, 0xf4,},
{0x30, 0xf8,},
{0x88, 0x02,}, //EXP Max(120Hz) 8.00 fps
{0x89, 0xdc,},
{0x8a, 0x6c,},
{0xa0, 0x02,}, //EXP Max(100Hz) 8.33 fps
{0xa1, 0xbf,},
{0xa2, 0x20,},
{0x03, 0x20,},
{0x10, 0x9c,}, //ae on
{0x03, 0x00,},
{0x01, 0x70,},
{0xff, 0x28,}, //delay 400ms
};


static struct sensor_reg  sr030pc50_start_stream[] = {
{0x03, 0x00,},
{0x01, 0x71,},
};

static struct sensor_reg  sr030pc50_stop_stream[] = {
{0x03, 0x00,},
{0x01, 0x70,},
};
#endif

static const struct sensor_regset_table regset_table = {
	.init			= SENSOR_REGISTER_REGSET(sr030_Init_Reg),
	.stop_stream		= SENSOR_REGISTER_REGSET(sr030_stop_stream),
	.effect_none		= SENSOR_REGISTER_REGSET(sr030_effect_none),
	.brightness_default	= SENSOR_REGISTER_REGSET(sr030_brightness_default),
	.wb_auto		= SENSOR_REGISTER_REGSET(sr030_wb_auto),
	.resol_640_480		= SENSOR_REGISTER_REGSET(sr030_resol_640_480)
};

static struct fimc_is_sensor_cfg settle_sr030[] = {
	/* 3264x2448@30fps */
	FIMC_IS_SENSOR_CFG(3264, 2448, 30, 17, 0, CSI_DATA_LANES_1),
};

static struct fimc_is_vci vci_sr030[] = {
	{
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.vc_map = {0, 1, 2, 3}
	}, {
		.pixelformat = V4L2_PIX_FMT_JPEG,
		.vc_map = {0, 1, 2, 3}
	}
};

static int fimc_is_sr030_read8(struct i2c_client *client,
	u8 addr, u8 *val)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 wbuf[2];

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	/* 1. I2C operation for writing. */
	msg[0].addr = client->addr;
	msg[0].flags = 0; /* write : 0, read : 1 */
	msg[0].len = 1;
	msg[0].buf = wbuf;
	/* TODO : consider other size of buffer */
	wbuf[0] = addr;

	/* 2. I2C operation for reading data. */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = val;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		err("i2c treansfer fail");
		goto p_err;
	}

#ifdef PRINT_I2CCMD
	info("I2CR08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, *val);
#endif

p_err:
	return ret;
}

static int fimc_is_sr030_write8(struct i2c_client *client,
	u8 addr, u8 val)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[3];

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = wbuf;
	wbuf[0] = addr;
	wbuf[1] = val;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

#ifdef PRINT_I2CCMD
	info("I2CW08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, val);
#endif

p_err:
	return ret;
}

static int sensor_sr030_apply_set(struct i2c_client *client,
	const struct sensor_regset *regset)
{
	int ret = 0;
	u32 i;

	FIMC_BUG(!client);
	FIMC_BUG(!regset);

	for (i = 0; i < regset->size; i++) {
		ret = fimc_is_sr030_write8(client, regset->reg[i].addr, regset->reg[i].data);
		if (unlikely(!ret)) {
			err("regs set(addr 0x%08X, size %d) apply is fail(%d)",
				(u32)&regset->reg[i], regset->size, ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int sensor_sr030_s_capture_mode(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_sr030_s_flash(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_sr030_s_autofocus(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_sr030_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	u8 id;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030->system_clock = 146 * 1000 * 1000;
	module_sr030->line_length_pck = 146 * 1000 * 1000;

	sensor_sr030_apply_set(client, &regset_table.init);
	sensor_sr030_apply_set(client, &regset_table.stop_stream);
	fimc_is_sr030_read8(client, 0x4, &id);

	info("[MOD:D] %s(id%X)\n", __func__, id);

p_err:
	return ret;
}

static int sensor_sr030_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_SCENEMODE:
	case V4L2_CID_FOCUS_MODE:
	case V4L2_CID_WHITE_BALANCE_PRESET:
	case V4L2_CID_IMAGE_EFFECT:
	case V4L2_CID_CAM_ISO:
	case V4L2_CID_CAM_CONTRAST:
	case V4L2_CID_CAM_SATURATION:
	case V4L2_CID_CAM_SHARPNESS:
	case V4L2_CID_CAM_BRIGHTNESS:
	case V4L2_CID_CAM_METERING:
	case V4L2_CID_CAM_SET_AUTO_FOCUS:
	case V4L2_CID_CAM_OBJECT_POSITION_X:
	case V4L2_CID_CAM_OBJECT_POSITION_Y:
	case V4L2_CID_CAM_FACE_DETECTION:
	case V4L2_CID_CAM_WDR:
	case V4L2_CID_CAM_AUTO_FOCUS_RESULT:
	case V4L2_CID_JPEG_QUALITY:
	case V4L2_CID_CAM_AEAWB_LOCK_UNLOCK:
	case V4L2_CID_CAM_CAF_START_STOP:
	case V4L2_CID_CAM_ZOOM:
		err("invalid ioctl(0x%08X) is requested", ctrl->id);
		break;
	case V4L2_CID_CAM_SINGLE_AUTO_FOCUS:
		ret = sensor_sr030_s_autofocus(subdev);
		if (ret) {
			err("sensor_sr030_s_autofocus is fail(%d)", ret);
			goto p_err;
		}
		break;
	case V4L2_CID_CAPTURE:
		ret = sensor_sr030_s_capture_mode(subdev, ctrl->value);
		if (ret) {
			err("sensor_sr030_s_flash is fail(%d)", ret);
			goto p_err;
		}
		break;
	case V4L2_CID_CAM_FLASH_MODE:
		ret = sensor_sr030_s_flash(subdev, ctrl->value);
		if (ret) {
			err("sensor_sr030_s_flash is fail(%d)", ret);
			goto p_err;
		}
		break;
	default:
		err("invalid ioctl(0x%08X) is requested", ctrl->id);
		ret = -EINVAL;
		break;
	}

p_err:
	return ret;
}

static int sensor_sr030_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_EXIF_EXPTIME:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_EXIF_FLASH:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_EXIF_TV:
		ctrl->value = 0;
		break;
	case V4L2_CID_CAMERA_EXIF_BV:
		ctrl->value = 0;
		break;
	default:
		err("invalid ioctl(0x%08X) is requested", ctrl->id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init		= sensor_sr030_init,
	.s_ctrl		= sensor_sr030_s_ctrl,
	.g_ctrl		= sensor_sr030_g_ctrl
};

static int sensor_sr030_s_format(struct v4l2_subdev *subdev, struct v4l2_mbus_framefmt *fmt)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);
	FIMC_BUG(!fmt);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!module) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030->image.window.offs_h = 0;
	module_sr030->image.window.offs_v = 0;
	module_sr030->image.window.width = fmt->width;
	module_sr030->image.window.height = fmt->height;
	module_sr030->image.window.o_width = fmt->width;
	module_sr030->image.window.o_height = fmt->height;
	module_sr030->image.format.pixelformat = fmt->code;

	info("[030] output size : %d x %d\n", fmt->width, fmt->height);

p_err:
	return ret;
}

static int sensor_sr030_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_module_enum *module;

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!module) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (enable) {
		ret = CALL_MOPS(module, stream_on, subdev);
		if (ret) {
			err("stream_on is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = CALL_MOPS(module, stream_off, subdev);
		if (ret) {
			err("stream_off is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return 0;
}

static int sensor_sr030_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;
	u64 framerate;

	FIMC_BUG(!subdev);
	FIMC_BUG(!param);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	if (!tpf->numerator) {
		err("numerator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	framerate = tpf->denominator;

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!module) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = CALL_MOPS(module, s_duration, subdev, framerate);
	if (ret) {
		err("s_duration is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_sr030_s_stream,
	.s_parm = sensor_sr030_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_sr030_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_sr030_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;
	u32 width, height;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	width = module_sr030->image.window.width;
	height = module_sr030->image.window.height;

	if ((width == 640) && (height == 480)) {
		sensor_sr030_apply_set(client, &regset_table.effect_none);
		sensor_sr030_apply_set(client, &regset_table.wb_auto);
		sensor_sr030_apply_set(client, &regset_table.brightness_default);
		sensor_sr030_apply_set(client, &regset_table.resol_640_480);
	} else {
		BUG();
	}

	info("[MOD:D] stream on\n");

p_err:
	return ret;
}

int sensor_sr030_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	info("[4EC] stream off\n");

p_err:
	return ret;
}

/*
 * @ brief
 * frame duration time
 * @ unit
 * nano second
 * @ remarks
 */
int sensor_sr030_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	int ret = 0;
	u32 framerate;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_sr030 *module_sr030;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_sr030 = module->private_data;
	if (unlikely(!module_sr030)) {
		err("module_sr030 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	framerate = duration;
	if (framerate > FRAME_RATE_MAX) {
		err("framerate is invalid(%d)", framerate);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int sensor_sr030_g_min_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_g_max_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
{
	int ret = 0;
	u8 value;
	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!sensor)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = sensor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	value = exposure & 0xFF;


p_err:
	return ret;
}

int sensor_sr030_g_min_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_g_max_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_s_again(struct v4l2_subdev *subdev, u64 sensitivity)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	return ret;
}

int sensor_sr030_g_min_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_g_max_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_s_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_g_min_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_sr030_g_max_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

struct fimc_is_sensor_ops module_sr030_ops = {
	.stream_on	= sensor_sr030_stream_on,
	.stream_off	= sensor_sr030_stream_off,
	.s_duration	= sensor_sr030_s_duration,
	.g_min_duration	= sensor_sr030_g_min_duration,
	.g_max_duration	= sensor_sr030_g_max_duration,
	.s_exposure	= sensor_sr030_s_exposure,
	.g_min_exposure	= sensor_sr030_g_min_exposure,
	.g_max_exposure	= sensor_sr030_g_max_exposure,
	.s_again	= sensor_sr030_s_again,
	.g_min_again	= sensor_sr030_g_min_again,
	.g_max_again	= sensor_sr030_g_max_again,
	.s_dgain	= sensor_sr030_s_dgain,
	.g_min_dgain	= sensor_sr030_g_min_dgain,
	.g_max_dgain	= sensor_sr030_g_max_dgain
};

int sensor_sr030_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	device = &core->sensor[SENSOR_SR030_INSTANCE];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	module->sensor_id = SENSOR_SR030_NAME;
	module->subdev = subdev_module;
	module->device = SENSOR_SR030_INSTANCE;
	module->ops = &module_sr030_ops;
	module->client = client;
	module->pixel_width = 1920 + 16;
	module->pixel_height = 1080 + 10;
	module->active_width = 1920;
	module->active_height = 1080;
	module->max_framerate = 30;
	module->position = SENSOR_POSITION_FRONT;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_1;
	module->bitwidth = 10;
	module->vcis = ARRAY_SIZE(vci_sr030);
	module->vci = vci_sr030;
	module->setfile_name = "setfile_sr325.bin";
	module->cfgs = ARRAY_SIZE(settle_sr030);
	module->cfg = settle_sr030;
	module->private_data = kzalloc(sizeof(struct fimc_is_module_sr030), GFP_KERNEL);
	if (!module->private_data) {
		err("private_data is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

#ifdef DEFAULT_S5K4EC_DRIVING
	v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);
#else
	v4l2_subdev_init(subdev_module, &subdev_ops);
#endif
	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->id);

p_err:
	info("%s(%d)\n", __func__, ret);
	return ret;
}

static int sensor_sr030_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_sr030_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-sensor-4ec",
	},
	{},
};
#endif

static const struct i2c_device_id sensor_sr030_idt[] = {
	{ SENSOR_NAME, 0 },
};

static struct i2c_driver sensor_sr030_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_sr030_match
#endif
	},
	.probe	= sensor_sr030_probe,
	.remove	= sensor_sr030_remove,
	.id_table = sensor_sr030_idt
};

static int __init sensor_sr030_load(void)
{
        return i2c_add_driver(&sensor_sr030_driver);
}

static void __exit sensor_sr030_unload(void)
{
        i2c_del_driver(&sensor_sr030_driver);
}

module_init(sensor_sr030_load);
module_exit(sensor_sr030_unload);

MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor sr325 driver");
MODULE_LICENSE("GPL v2");
