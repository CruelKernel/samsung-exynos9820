/*
 * Copyright (C) 2017 Semtech Corporation. All rights reserved.
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
#ifndef _SX9330_I2C_REG_H_
#define _SX9330_I2C_REG_H_

/*
 *  I2C Registers
 */
#define   SX9330_HOSTIRQSRC_REG 	0x4000
#define   SX9330_HOSTIRQMSK_REG  	0x4004
#define   SX9330_HOSTIRQCTRL_REG  	0x4008
#define   SX9330_PAUSESTAT_REG  	0x4010

#define   SX9330_AFECTRL_REG		0x4054

#define   SX9330_PWM_REG  		0x4080 
#define   SX9330_CLKGEN_REG      	0x4200 

#define   SX9330_I2CADDR_REG		0x41C4
#define   SX9330_RESET_REG		0x4240
#define   SX9330_CMD_REG		0x4280
#define   SX9330_TOPSTAT0_REG		0x4284

#define   SX9330_PINCFG_REG  		0x42C0
#define   SX9330_PINDOUT_REG		0x42C4
#define   SX9330_PINDIN_REG		0x42C8

#define   SX9330_INFO_REG		0x42D8

#define   SX9330_STAT0_REG		0x8000
#define   SX9330_STAT1_REG		0x8004
#define   SX9330_STAT2_REG		0x8008

#define   SX9330_IRQCFG0_REG      	0x800C
#define   SX9330_IRQCFG1_REG		0x8010
#define   SX9330_IRQCFG2_REG		0x8014
#define   SX9330_IRQCFG3_REG		0x8018

#define   SX9330_SCANPERIOD_REG		0x801C
#define   SX9330_GNRLCTRL2_REG		0x8020
#define   SX9330_AFEPARAMSPH0_REG       0x8024
#define   SX9330_AFEPHPH0_REG		0x8028
#define   SX9330_AFEPARAMSPH1_REG      	0x802C
#define   SX9330_AFEPHPH1_REG		0x8030
#define   SX9330_AFEPARAMSPH2_REG      	0x8034
#define   SX9330_AFEPHPH2_REG		0x8038
#define   SX9330_AFEPARAMSPH3_REG       0x803C
#define   SX9330_AFEPHPH3_REG		0x8040
#define   SX9330_AFEPARAMSPH4_REG       0x8044
#define   SX9330_AFEPHPH4_REG		0x8048
#define   SX9330_AFEPARAMSPH5_REG       0x804c
#define   SX9330_AFEPHPH5_REG		0x8050
#define   SX9330_AFEPARAMSPH6_REG       0x8054
#define   SX9330_AFEPHPH6_REG		0x8058
#define   SX9330_AFEPARAMSPH7_REG       0x805c
#define   SX9330_AFEPHPH7_REG		0x8060

#define   SX9330_ADCFILTPH0_REG		0x8064
#define   SX9330_AVGBFILTPH0_REG      	0x8068
#define   SX9330_AVGAFILTPH0_REG      	0x806C
#define   SX9330_ADVDIG0PH0_REG         0x8070
#define   SX9330_ADVDIG1PH0_REG         0x8074
#define   SX9330_ADVDIG2PH0_REG         0x8078
#define   SX9330_ADVDIG3PH0_REG         0x807C
#define   SX9330_ADVDIG4PH0_REG         0x8080
#define   SX9330_ADCFILTPH1_REG       	0x8084
#define   SX9330_AVGBFILTPH1_REG      	0x8088
#define   SX9330_AVGAFILTPH1_REG      	0x808C
#define   SX9330_ADVDIG0PH1_REG         0x8090
#define   SX9330_ADVDIG1PH1_REG         0x8094
#define   SX9330_ADVDIG2PH1_REG         0x8098
#define   SX9330_ADVDIG3PH1_REG         0x809C
#define   SX9330_ADVDIG4PH1_REG         0x80A0
#define   SX9330_ADCFILTPH2_REG       	0x80A4
#define   SX9330_AVGBFILTPH2_REG      	0x80A8
#define   SX9330_AVGAFILTPH2_REG      	0x80AC
#define   SX9330_ADVDIG0PH2_REG         0x80B0
#define   SX9330_ADVDIG1PH2_REG         0x80B4
#define   SX9330_ADVDIG2PH2_REG         0x80B8
#define   SX9330_ADVDIG3PH2_REG         0x80BC
#define   SX9330_ADVDIG4PH2_REG         0x80C0
#define   SX9330_ADCFILTPH3_REG      	0x80C4
#define   SX9330_AVGBFILTPH3_REG     	0x80C8
#define   SX9330_AVGAFILTPH3_REG     	0x80CC
#define   SX9330_ADVDIG0PH3_REG         0x80D0
#define   SX9330_ADVDIG1PH3_REG         0x80D4
#define   SX9330_ADVDIG2PH3_REG         0x80D8
#define   SX9330_ADVDIG3PH3_REG         0x80DC
#define   SX9330_ADVDIG4PH3_REG         0x80E0
#define   SX9330_ADCFILTPH4_REG       	0x80E4
#define   SX9330_AVGBFILTPH4_REG      	0x80E8
#define   SX9330_AVGAFILTPH4_REG      	0x80EC
#define   SX9330_ADVDIG0PH4_REG         0x80F0
#define   SX9330_ADVDIG1PH4_REG         0x80F4
#define   SX9330_ADVDIG2PH4_REG         0x80F8
#define   SX9330_ADVDIG3PH4_REG         0x80FC
#define   SX9330_ADVDIG4PH4_REG         0x8100
#define   SX9330_ADCFILTPH5_REG       	0x8104
#define   SX9330_AVGBFILTPH5_REG      	0x8108
#define   SX9330_AVGAFILTPH5_REG      	0x810C
#define   SX9330_ADVDIG0PH5_REG         0x8110
#define   SX9330_ADVDIG1PH5_REG         0x8114
#define   SX9330_ADVDIG2PH5_REG         0x8118
#define   SX9330_ADVDIG3PH5_REG         0x811C
#define   SX9330_ADVDIG4PH5_REG       	0x8120
#define   SX9330_ADCFILTPH6_REG       	0x8124
#define   SX9330_AVGBFILTPH6_REG      	0x8128
#define   SX9330_AVGAFILTPH6_REG      	0x812C
#define   SX9330_ADVDIG0PH6_REG         0x8130
#define   SX9330_ADVDIG1PH6_REG         0x8134
#define   SX9330_ADVDIG2PH6_REG         0x8138
#define   SX9330_ADVDIG3PH6_REG         0x813C
#define   SX9330_ADVDIG4PH6_REG         0x8140
#define   SX9330_ADCFILTPH7_REG       	0x8144
#define   SX9330_AVGBFILTPH7_REG      	0x8148
#define   SX9330_AVGAFILTPH7_REG      	0x814C
#define   SX9330_ADVDIG0PH7_REG         0x8150
#define   SX9330_ADVDIG1PH7_REG         0x8154
#define   SX9330_ADVDIG2PH7_REG         0x8158
#define   SX9330_ADVDIG3PH7_REG         0x815C
#define   SX9330_ADVDIG4PH7_REG         0x8160

#define   SX9330_STEPCANCEL0A_REG     	0x8164
#define   SX9330_STEPCANCEL1A_REG     	0x8168
#define   SX9330_STEPCANCEL0B_REG     	0x816C
#define   SX9330_STEPCANCEL1B_REG     	0x8170
			
#define   SX9330_REFCORRA_REG         	0x8174
#define   SX9330_REFCORRB_REG         	0x8178
			
#define   SX9330_SMARTSAR0A_REG       	0x817C
#define   SX9330_SMARTSAR1A_REG         0x8180
#define   SX9330_SMARTSAR2A_REG         0x8184
#define   SX9330_SMARTSAR3A_REG         0x8188
#define   SX9330_SMARTSAR4A_REG         0x818C
#define   SX9330_SMARTSAR0B_REG         0x8190
#define   SX9330_SMARTSAR1B_REG         0x8194
#define   SX9330_SMARTSAR2B_REG         0x8198
#define   SX9330_SMARTSAR3B_REG       	0x819C
#define   SX9330_SMARTSAR4B_REG         0x81A0

#define   SX9330_PROX2PWMA_REG          0x81A4

#define   SX9330_AUTOFREQ0_REG       	0x81AC
#define   SX9330_AUTOFREQ1_REG       	0x81B0
			
#define   SX9330_USEPH0_REG          	0x81B4
#define   SX9330_USEPH1_REG          	0x81B8
#define   SX9330_USEPH2_REG          	0x81BC
#define   SX9330_USEPH3_REG          	0x81C0
#define   SX9330_USEPH4_REG          	0x81C4
#define   SX9330_USEPH5_REG          	0x81C8
#define   SX9330_USEPH6_REG          	0x81CC
#define   SX9330_USEPH7_REG          	0x81D0
#define   SX9330_AVGPH0_REG          	0x81D4
#define   SX9330_AVGPH1_REG          	0x81D8
#define   SX9330_AVGPH2_REG          	0x81DC
#define   SX9330_AVGPH3_REG          	0x81E0
#define   SX9330_AVGPH4_REG          	0x81E4
#define   SX9330_AVGPH5_REG          	0x81E8
#define   SX9330_AVGPH6_REG          	0x81EC
#define   SX9330_AVGPH7_REG          	0x81F0
#define   SX9330_DIFFPH0_REG         	0x81F4
#define   SX9330_DIFFPH1_REG         	0x81F8
#define   SX9330_DIFFPH2_REG         	0x81FC
#define   SX9330_DIFFPH3_REG         	0x8200
#define   SX9330_DIFFPH4_REG         	0x8204
#define   SX9330_DIFFPH5_REG         	0x8208
#define   SX9330_DIFFPH6_REG         	0x820C
#define   SX9330_DIFFPH7_REG         	0x8210

#define   SX9330_DBGVARSEL_REG		0X8214

#define   SX9330_OFFSETPH0_REG         	SX9330_AFEPHPH0_REG 	//bit14:0
#define   SX9330_OFFSETPH1_REG         	SX9330_AFEPHPH1_REG	//bit14:0
#define   SX9330_OFFSETPH2_REG         	SX9330_AFEPHPH2_REG	//bit14:0
#define   SX9330_OFFSETPH3_REG         	SX9330_AFEPHPH3_REG	//bit14:0
#define   SX9330_OFFSETPH4_REG         	SX9330_AFEPHPH4_REG	//bit14:0
#define   SX9330_OFFSETPH5_REG         	SX9330_AFEPHPH5_REG	//bit14:0
#define   SX9330_OFFSETPH6_REG         	SX9330_AFEPHPH6_REG	//bit14:0
#define   SX9330_OFFSETPH7_REG         	SX9330_AFEPHPH7_REG	//bit14:0

//i2c register bit mask
#define   MSK_IRQSTAT_RESET    		0x00000080
#define   MSK_IRQSTAT_TOUCH    		0x00000040
#define   MSK_IRQSTAT_RELEASE  		0x00000020
#define   MSK_IRQSTAT_COMP     		0x00000010
#define   MSK_IRQSTAT_CONV     		0x00000008
#define   MSK_IRQSTAT_PROG2IRQ 		0x00000004
#define   MSK_IRQSTAT_PROG1IRQ 		0x00000002
#define   MSK_IRQSTAT_PROG0IRQ 		0x00000001
     
#define   I2C_SOFTRESET_VALUE 		0x000000DE

#define   I2C_REGCMD_PHEN		0x0000000F
#define   I2C_REGCMD_COMPEN		0x0000000E
#define   I2C_REGCMD_EN_SLEEP		0x0000000D
#define   I2C_REGCMD_EX_SLEEP		0x0000000C
        
#define   I2C_REGGNRLCTRL2_PHEN_MSK 	0x000000FF
#define   I2C_REGGNRLCTRL2_COMPEN_MSK 	0x00ff0000

#define   MSK_TOPSTAT0_CMDBUSY		0x0001
#define   MSK_STAT1_CONVSTAT       	((long int)0x01<<7)

#define   MSK_REGSTAT0_STEADYSTAT7  	0x80
#define   MSK_REGSTAT0_STEADYSTAT6  	0x40
#define   MSK_REGSTAT0_STEADYSTAT5  	0x20
#define   MSK_REGSTAT0_STEADYSTAT4  	0x10
#define   MSK_REGSTAT0_STEADYSTAT3  	0x08
#define   MSK_REGSTAT0_STEADYSTAT2  	0x04
#define   MSK_REGSTAT0_STEADYSTAT1  	0x02
#define   MSK_REGSTAT0_STEADYSTAT0  	0x01
			
#define   MSK_REGSTAT0_BODYSTAT7    	((long int)0x80<<8)
#define   MSK_REGSTAT0_BODYSTAT6    	((long int)0x40<<8)
#define   MSK_REGSTAT0_BODYSTAT5    	((long int)0x20<<8)
#define   MSK_REGSTAT0_BODYSTAT4    	((long int)0x10<<8)
#define   MSK_REGSTAT0_BODYSTAT3    	((long int)0x08<<8)
#define   MSK_REGSTAT0_BODYSTAT2    	((long int)0x04<<8)
#define   MSK_REGSTAT0_BODYSTAT1    	((long int)0x02<<8)
#define   MSK_REGSTAT0_BODYSTAT0    	((long int)0x01<<8)
		
#define   MSK_REGSTAT0_TABLESTAT7   	((long int)0x80<<16)
#define   MSK_REGSTAT0_TABLESTAT6   	((long int)0x40<<16)
#define   MSK_REGSTAT0_TABLESTAT5   	((long int)0x20<<16)
#define   MSK_REGSTAT0_TABLESTAT4   	((long int)0x10<<16)
#define   MSK_REGSTAT0_TABLESTAT3   	((long int)0x08<<16)
#define   MSK_REGSTAT0_TABLESTAT2   	((long int)0x04<<16)
#define   MSK_REGSTAT0_TABLESTAT1   	((long int)0x02<<16)
#define   MSK_REGSTAT0_TABLESTAT0   	((long int)0x01<<16)
		
#define   MSK_REGSTAT0_PROXSTAT7    	((long int)0x80<<24)
#define   MSK_REGSTAT0_PROXSTAT6    	((long int)0x40<<24)
#define   MSK_REGSTAT0_PROXSTAT5    	((long int)0x20<<24)
#define   MSK_REGSTAT0_PROXSTAT4    	((long int)0x10<<24)
#define   MSK_REGSTAT0_PROXSTAT3    	((long int)0x08<<24)
#define   MSK_REGSTAT0_PROXSTAT2    	((long int)0x04<<24)
#define   MSK_REGSTAT0_PROXSTAT1    	((long int)0x02<<24)
#define   MSK_REGSTAT0_PROXSTAT0    	((long int)0x01<<24)                   

#define   MSK_REGSTAT1_FAILSTAT7    	((long int)0x80<<24)
#define   MSK_REGSTAT1_FAILSTAT6    	((long int)0x40<<24)
#define   MSK_REGSTAT1_FAILSTAT5    	((long int)0x20<<24)
#define   MSK_REGSTAT1_FAILSTAT4    	((long int)0x10<<24)
#define   MSK_REGSTAT1_FAILSTAT3    	((long int)0x08<<24)
#define   MSK_REGSTAT1_FAILSTAT2    	((long int)0x04<<24)
#define   MSK_REGSTAT1_FAILSTAT1    	((long int)0x02<<24)
#define   MSK_REGSTAT1_FAILSTAT0    	((long int)0x01<<24)

#define   MSK_REGSTAT1_COMPSTAT7   	((long int)0x80<<16)
#define   MSK_REGSTAT1_COMPSTAT6   	((long int)0x40<<16)
#define   MSK_REGSTAT1_COMPSTAT5   	((long int)0x20<<16)
#define   MSK_REGSTAT1_COMPSTAT4   	((long int)0x10<<16)
#define   MSK_REGSTAT1_COMPSTAT3   	((long int)0x08<<16)
#define   MSK_REGSTAT1_COMPSTAT2   	((long int)0x04<<16)
#define   MSK_REGSTAT1_COMPSTAT1   	((long int)0x02<<16)
#define   MSK_REGSTAT1_COMPSTAT0   	((long int)0x01<<16)                             

#define   MSK_REGSTAT1_SATSTAT7    	((long int)0x80<<8)
#define   MSK_REGSTAT1_SATSTAT6    	((long int)0x40<<8)
#define   MSK_REGSTAT1_SATSTAT5    	((long int)0x20<<8)
#define   MSK_REGSTAT1_SATSTAT4    	((long int)0x10<<8)
#define   MSK_REGSTAT1_SATSTAT3    	((long int)0x08<<8)
#define   MSK_REGSTAT1_SATSTAT2    	((long int)0x04<<8)
#define   MSK_REGSTAT1_SATSTAT1    	((long int)0x02<<8)
#define   MSK_REGSTAT1_SATSTAT0    	((long int)0x01<<8)

#define   MSK_REGSTAT1_STEPSTATB    	((long int)0x02<<5)
#define   MSK_REGSTAT1_STEPSTATA    	((long int)0x01<<5)

#define   MSK_REGSTAT2_STARTUPSTAT7    	((long int)0x80)
#define   MSK_REGSTAT2_STARTUPSTAT6    	((long int)0x40)
#define   MSK_REGSTAT2_STARTUPSTAT5    	((long int)0x20)
#define   MSK_REGSTAT2_STARTUPSTAT4    	((long int)0x10)
#define   MSK_REGSTAT2_STARTUPSTAT3    	((long int)0x08)
#define   MSK_REGSTAT2_STARTUPSTAT2    	((long int)0x04)
#define   MSK_REGSTAT2_STARTUPSTAT1    	((long int)0x02)
#define   MSK_REGSTAT2_STARTUPSTAT0    	((long int)0x01)

#define   MSK_REGSTAT1_STEADYSTATALL 	0x02

#define   MSK_REG_STEP_CANCEL0_A_PH  	((long int)0x07<<26)
#define   MSK_REG_STEP_CANCEL0_B_PH  	((long int)0x07<<26)

#define   SX9330_STAT0_PROXSTAT_PH7_FLAG  0x80
#define   SX9330_STAT0_PROXSTAT_PH6_FLAG  0x40
#define   SX9330_STAT0_PROXSTAT_PH5_FLAG  0x20
#define   SX9330_STAT0_PROXSTAT_PH4_FLAG  0x10
#define   SX9330_STAT0_PROXSTAT_PH3_FLAG  0x08
#define   SX9330_STAT0_PROXSTAT_PH2_FLAG  0x04
#define   SX9330_STAT0_PROXSTAT_PH1_FLAG  0x02
#define   SX9330_STAT0_PROXSTAT_PH0_FLAG  0x01

struct smtc_reg_data {
    unsigned short reg;
    unsigned int val;
};

enum {
	SX9330_SCANPERIOD_REG_IDX = 10,
	SX9330_GNRLCTRL2_REG_IDX = 11,
	SX9330_AFEPARAMSPH0_REG_IDX = 12,
	SX9330_AFEPHPH0_REG_IDX = 13,
	SX9330_ADCFILTPH0_REG_IDX = 28,
	SX9330_AVGBFILTPH0_REG_IDX = 29,
	SX9330_AVGAFILTPH0_REG_IDX = 30,
	SX9330_ADVDIG3PH0_REG_IDX = 34,
	SX9330_ADVDIG4PH0_REG_IDX = 35,
	SX9330_REFCORRA_REG_IDX = 96
};

/* for device tree parse */
#define SX9330_SCANPERIOD	"sx9330,scanperiod_reg"
#define SX9330_GNRLCTRL2	"sx9330,gnrlctrl2_reg"
#define SX9330_AFEPARAMSPH0	"sx9330,afeparamsph0_reg"
#define SX9330_AFEPHPH0		"sx9330,afephph0_reg"
#define SX9330_ADCFILTPH0	"sx9330,adcfiltph0_reg"
#define SX9330_AFEPARAMSPH1	"sx9330,afeparamsph1_reg"
#define SX9330_ADCFILTPH1	"sx9330,adcfiltph1_reg"
#define SX9330_AVGBFILT		"sx9330,avgbfilt_reg"
#define SX9330_AVGAFILT		"sx9330,avgafilt_reg"
#define SX9330_ADVDIG3		"sx9330,advdig3_reg"
#define SX9330_ADVDIG4		"sx9330,advdig4_reg"
#define SX9330_REFCORRA		"sx9330,refcorra_reg"

/*define the value without Phase enable settings for easy changes in driver*/
static struct smtc_reg_data setup_reg[] = {
    {
        .reg = SX9330_CMD_REG,
        .val = I2C_REGCMD_PHEN,
    },
    {
        .reg = SX9330_AFECTRL_REG,
        .val = 0x00000400,
    },
    {
        .reg = SX9330_PWM_REG,
        .val = 0x80804000,
    },
    {
        .reg = SX9330_CLKGEN_REG,
        .val = 0x00000008,
    },
    {
        .reg = SX9330_PINCFG_REG,
        .val = 0x08000000,
    },
    {
        .reg = SX9330_PINDOUT_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_IRQCFG0_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_IRQCFG1_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_IRQCFG2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_IRQCFG3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SCANPERIOD_REG,
        .val = 0x00000019,	/* [SCANPERIOD : 2048] */
    },
    {
        .reg = SX9330_GNRLCTRL2_REG,
        .val = 0x00FF0003,	/* [COMPEN : xFF] [PHEN : 0, 1] */
    },
    {
        .reg = SX9330_AFEPARAMSPH0_REG,
        .val = 0x00000405,	/* [FREQ : 250Hz] [RESOLUTION : 256] */
    },
    {
        .reg = SX9330_AFEPHPH0_REG,
        .val = 0x00028000,	/* [CSIO0 Measured Input] */
    },
    {
        .reg = SX9330_AFEPARAMSPH1_REG,
        .val = 0x10000227,	/* [RINT_PH1 : LOW] [AGAIN_PH1 : 1.1pF] [FREQ_PH1 : 125.33kHz] [RESOLUTION : 1024] */
    },
    {
        .reg = SX9330_AFEPHPH1_REG,
        .val = 0x00140000,	/* [CSIO1 Measured Input] */
    },
    {
        .reg = SX9330_AFEPARAMSPH2_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AFEPARAMSPH3_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AFEPARAMSPH4_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AFEPARAMSPH5_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AFEPARAMSPH6_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AFEPARAMSPH7_REG,
        .val = 0x00000444,
    },
    {
        .reg = SX9330_AFEPHPH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH0_REG,
        .val = 0x00700000,	/* [RAWFILT : 1-1/128] */
    },
    {
        .reg = SX9330_AVGBFILTPH0_REG,
        .val = 0x60600B00,
    },
    {
        .reg = SX9330_AVGAFILTPH0_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH0_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH0_REG,
        .val = 0x706447,
    },
    {
        .reg = SX9330_ADVDIG2PH0_REG,
        .val = 0x15050200,
    },
    {
        .reg = SX9330_ADVDIG3PH0_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH0_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH1_REG,
        .val = 0x002A5F10,	/* [RAWFILT : 1-1/4] [ADCFILTSAMPLES : 1-1/4] [ADCFILTCOEF : 1/4] [PROXTHRESH : 4512] [HYST_PH : +/-6%] */
    },
    {
        .reg = SX9330_AVGBFILTPH1_REG,
        .val = 0x20600080,	/* [AVGNEGILT : off] [AVGPOSFILT : off] [USETHRSHNODET : 5] */
    },
    {
        .reg = SX9330_AVGAFILTPH1_REG,
        .val = 0x40C0170A,	/* [USETHRSHDETPOS : 4] [USETHRSHDETNEG : -4] [USECORRDETNEG : -6] [USEFILTENABLE : on] [USECORRNODET : 7] [USECORRDETNEG : -6] */
    },
    {
        .reg = SX9330_ADVDIG0PH1_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH1_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH1_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH1_REG,
        .val = 0x21002100,	/* [REFCOEFINCA : 2] [REFCOEFDECA : 2] */
    },
    {
        .reg = SX9330_ADVDIG4PH1_REG,
        .val = 0x00010000,	/* [REFCORRENABLE : Engine A only] */
    },
    {
        .reg = SX9330_ADCFILTPH2_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH2_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH2_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH2_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH2_REG,
        .val = 0x00000000,
    },
        {
        .reg = SX9330_ADCFILTPH3_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH3_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH3_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH3_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH4_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH4_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH4_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH4_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH5_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH5_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH5_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH5_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH6_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH6_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH6_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH6_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADCFILTPH7_REG,
        .val = 0x100000,
    },
    {
        .reg = SX9330_AVGBFILTPH7_REG,
        .val = 0x20600C00,
    },
    {
        .reg = SX9330_AVGAFILTPH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG0PH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG1PH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG2PH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG3PH7_REG ,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_ADVDIG4PH7_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_STEPCANCEL0A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_STEPCANCEL1A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_STEPCANCEL0B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_STEPCANCEL1B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_REFCORRA_REG,
        .val = 0x04000008,	/* [REFENABLE : ON, engine A] [REFINIT : b1] */
    },
    {
        .reg = SX9330_REFCORRB_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR0A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR1A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR2A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR3A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR4A_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR0B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR1B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR2B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR3B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_SMARTSAR4B_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_PROX2PWMA_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_AUTOFREQ0_REG,
        .val = 0x000000FF,
    },
    {
        .reg = SX9330_AUTOFREQ1_REG,
        .val = 0x0000000E,
    },
    {
        .reg = SX9330_HOSTIRQMSK_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_HOSTIRQCTRL_REG,
        .val = 0x00000000,
    },
    {
        .reg = SX9330_DBGVARSEL_REG,
        .val = 0x01B00008,	/* [PHASESEL : 1] */
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

#endif /* _SX9330_I2C_REG_H_*/
