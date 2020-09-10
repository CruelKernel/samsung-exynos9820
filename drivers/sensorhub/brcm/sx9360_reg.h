/*
 * Copyright (C) 2018 Semtech Corporation. All rights reserved.
 *
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef _SX9360_I2C_REG_H_
#define _SX9360_I2C_REG_H_

/*
 *  I2C Registers
 */
enum registers1 {
    SX9360_IRQSTAT_REG = 0x00,
    SX9360_STAT_REG,
    SX9360_IRQ_ENABLE_REG,
    SX9360_IRQCFG_REG,
    /* General Control */
    SX9360_GNRLCTRL0_REG = 0x10,
    SX9360_GNRLCTRL1_REG,
    SX9360_GNRLCTRL2_REG,
    /* Analog-Front-End (AFE) Control */
    SX9360_AFECTRL1_REG = 0x21,
    SX9360_AFEPARAM0PHR_REG, 
    SX9360_AFEPARAM1PHR_REG,
    SX9360_AFEPARAM0PHM_REG,
    SX9360_AFEPARAM1PHM_REG, 
    /* PROX Data update */
    SX9360_PROXCTRL0PHR_REG = 0x40,
    SX9360_PROXCTRL0PHM_REG,
    SX9360_PROXCTRL1_REG,
    SX9360_PROXCTRL2_REG,
    SX9360_PROXCTRL3_REG,
    SX9360_PROXCTRL4_REG,
    SX9360_PROXCTRL5_REG,
    /* Reference sensor correction */
    SX9360_REFCORR0_REG = 0x60,
    SX9360_REFCORR1_REG,
    /* USE Filter - Main phase only */
    SX9360_USEFILTER0_REG =0x70,
    SX9360_USEFILTER1_REG,
    SX9360_USEFILTER2_REG,
    SX9360_USEFILTER3_REG,
    SX9360_USEFILTER4_REG,
    /* Sensor Data Readback */
    SX9360_REGUSEMSBPHR = 0x90,
    SX9360_REGUSELSBPHR,
    SX9360_REGOFFSETMSBPHR,
    SX9360_REGOFFSETLSBPHR,
    SX9360_REGUSEMSBPHM,
    SX9360_REGUSELSBPHM,
    SX9360_REGAVGMSBPHM,
    SX9360_REGAVGLSBPHM,
    SX9360_REGDIFFMSBPHM,
    SX9360_REGDIFFLSBPHM,
    SX9360_REGOFFSETMSBPHM,
    SX9360_REGOFFSETLSBPHM,
    /*DeltaVar value of USE filter */
    SX9360_USEFILTMSB = 0x9E,
    SX9360_USEFILTLSB,
    /* Miscellaneous */
    SX9360_SOFTRESET_REG = 0xCF,
    SX9360_WHOAMI_REG = 0xFA,
    SX9360_REV_REG = 0xFE,
};

/* IrqStat 0:Inactive 1:Active */
#define SX9360_IRQSTAT_RESET_FLAG		0x10
#define SX9360_IRQSTAT_TOUCH_FLAG		0x08
#define SX9360_IRQSTAT_RELEASE_FLAG		0x04
#define SX9360_IRQSTAT_COMPDONE_FLAG		0x02
#define SX9360_IRQSTAT_CONV_FLAG		0x01

/* CpsStat */
#define SX9360_STAT_PROXSTAT_FLAG  		0x08

/* SoftReset */
#define SX9360_SOFTRESET  			0xDE

/* Manual Compensation */
#define SX9360_STAT_COMPSTAT_PHM 		0x04
#define SX9360_STAT_COMPSTAT_PHR 		0x02
#define SX9360_STAT_COMPSTAT_ALL_FLAG ( SX9360_STAT_COMPSTAT_PHM | SX9360_STAT_COMPSTAT_PHR )

/* Who Am I */
#define WHO_AM_I 96 // 0x60

struct smtc_reg_data {
    unsigned char reg;
    unsigned char val;
};

enum {
	SX9360_REFRESOLUTION_REG_IDX = 4,
	SX9360_REFAGAINFREQ_REG_IDX = 5,
	SX9360_RESOLUTION_REG_IDX = 6,
	SX9360_AGAINFREQ_REG_IDX = 7,
	SX9360_REFGAINRAWFILT_REG_IDX = 8,
	SX9360_GAINRAWFILT_REG_IDX = 9,
	SX9360_HYST_REG_IDX = 13,
	SX9360_PROXTHRESH_REG_IDX = 14,
};

/* for device tree parse */
#define SX9360_REFRESOLUTION	"sx9360,refresolution_reg"
#define SX9360_REFAGAINFREQ	"sx9360,refagainfreq_reg"
#define SX9360_RESOLUTION	"sx9360,resolution_reg"
#define SX9360_AGAINFREQ	"sx9360,againfreq_reg"
#define SX9360_REFGAINRAWFILT	"sx9360,refgainrawfilt_reg"
#define SX9360_GAINRAWFILT	"sx9360,gainrawfilt_reg"
#define SX9360_HYST		"sx9360,hyst_reg"
#define SX9360_PROXTHRESH	"sx9360,proxthresh_reg"
#define SX9360_PROXTHRESH_MCC	"sx9360,proxthresh_mcc"

/*define the value without Phase enable settings for easy changes in driver*/
#define SX9360_GNRLCTRL0_VAL_PHOFF (0x00)    
static struct smtc_reg_data setup_reg[] = {
	/* 0x10~0x12, General Control*/
    {
        .reg = SX9360_GNRLCTRL0_REG,
        .val = SX9360_GNRLCTRL0_VAL_PHOFF | 0x03,//PHEN
    },
    {
        .reg = SX9360_GNRLCTRL1_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_GNRLCTRL2_REG,
        .val = 0x32,
    },
    /* 0x21~0x25, Analog-Front-End (AFE) Control */
    {
        .reg = SX9360_AFECTRL1_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_AFEPARAM0PHR_REG,
        .val = 0x0E, //Resolution=512
    },
    {
        .reg = SX9360_AFEPARAM1PHR_REG,
        .val = 0x46,
    },
    {
        .reg = SX9360_AFEPARAM0PHM_REG,
        .val = 0x0E, //Resolution=512
    },
    {
        .reg = SX9360_AFEPARAM1PHM_REG,
        .val = 0x46,
    },
    /* 0x40~0x46, PROX Data update */
    {
        .reg = SX9360_PROXCTRL0PHR_REG,
        .val = 0x09,
    },
    {
        .reg = SX9360_PROXCTRL0PHM_REG,
        .val = 0x09,
    },
    {
        .reg = SX9360_PROXCTRL1_REG,
        .val = 0x20,
    },
    {
        .reg = SX9360_PROXCTRL2_REG,
        .val = 0x60,
    },
    {
        .reg = SX9360_PROXCTRL3_REG,
        .val = 0x0C,
    },
    {
        .reg = SX9360_PROXCTRL4_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_PROXCTRL5_REG,
        .val = 0x7E, //Threshold=7938
    },
    /* 0x60~0x61, Reference sensor correction */
    {
        .reg = SX9360_REFCORR0_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_REFCORR1_REG,
        .val = 0x00,
    },
    /* 0x70~0x74, USE Filter - Main phase only */
    {
        .reg = SX9360_USEFILTER0_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_USEFILTER1_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_USEFILTER2_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_USEFILTER3_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_USEFILTER4_REG,
        .val = 0x00,
    },
    /* 0x02~0x03, Interrupt */ 
    {
        .reg = SX9360_IRQ_ENABLE_REG,
        .val = 0x00,
    },
    {
        .reg = SX9360_IRQCFG_REG,
        .val = 0x00,
    },
};

enum {
    OFF = 0,
    ON = 1
};

extern int sensors_create_symlink(struct input_dev *inputdev);
extern void sensors_remove_symlink(struct input_dev *inputdev);
extern int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);

#endif /* _SX9360_I2C_REG_H_*/
