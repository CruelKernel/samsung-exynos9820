/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_regs.h
 *
 *	Description : register map header file
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *	History :
 *	----------------------------------------------------------------------
 */

#ifndef __FC8080_REGS_H__
#define __FC8080_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

	/* INTERFACE */
#if defined(CONFIG_TDMB_SPI)
#define FC8080_SPI
#elif defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
#define FC8080_I2C
#elif defined(CONFIG_TDMB_EBI)
#define FC8080_PPI
#endif

#ifdef FC8080_I2C
/* #define FIC_USE_I2C */
#endif

#ifdef CONFIG_TDMB_XTAL_FREQ
extern u32 main_xtal_freq;
#else
	/* X-TAL Frequency Configuration */
/*#define FC8080_FREQ_XTAL  16000*/
/*#define FC8080_FREQ_XTAL  16384*/
/*#define FC8080_FREQ_XTAL  19200*/
/*#define FC8080_FREQ_XTAL  24000*/
#define FC8080_FREQ_XTAL  24576
/*#define FC8080_FREQ_XTAL  26000*/
/*#define FC8080_FREQ_XTAL  27000*/
/*#define FC8080_FREQ_XTAL  27120*/
/*#define FC8080_FREQ_XTAL  32000*/
/*#define FC8080_FREQ_XTAL  38400*/
#endif

	/* INTERRUPT SOURCE */
#define BBM_MF_INT                  0x0001
#define BBM_WAGC_INT                0x0002
#define BBM_RECFG_INT               0x0004
#define BBM_TII_INT                 0x0008
#define BBM_SYNC_INT                0x0010
#define BBM_I2C_INT                 0x0020

	/* COMMON */
#define BBM_AP2APB_LT               0x0000
#define BBM_MD_RESET                0x0001
#define BBM_MD_INT_CLR              0x0002
#define BBM_MD_INT_EN               0x0003
#define BBM_MD_INT_STATUS           0x0004
#define BBM_RD_FIC                  0x0007
#define BBM_RD_BUF0                 0x0008
#define BBM_RD_BUF1                 0x0009
#define BBM_RD_BUF2                 0x000a
#define BBM_RD_BUF3                 0x000b
#define BBM_TSO_CLKDIV              0x0010
#define BBM_TSO_CTRL                0x0011
#define BBM_TSO_SELREG              0x0012
#define BBM_TSO_PAUSE               0x0013
#define BBM_CHIP_ID                 0x0018
#define BBM_RF_BYPASS               0x001f
#define BBM_TSGEN_GAP               0x0020
#define BBM_LDO_VCTRL               0x0031
#define BBM_XTAL_CCTRL              0x0032
#define BBM_RF_XTAL_EN              0x0033
#define BBM_RFRST_HIGH_HOLD         0x0035
#define BBM_ADC_OPMODE              0x0036
#define BBM_DM_CTRL                 0x0039
#define BBM_OVERRUN_GAP             0x003c

	/* RxFRONT */
#define BBM_SYNC_RST_REQ_EN         0x0100
#define BBM_QDD_COMMAND             0x0104
#define BBM_DCE_CTRL                0x0111
#define BBM_COEF00                  0x0120
#define BBM_COEF01                  0x0121
#define BBM_COEF02                  0x0122
#define BBM_COEF03                  0x0123
#define BBM_COEF04                  0x0124
#define BBM_COEF05                  0x0125
#define BBM_COEF06                  0x0126
#define BBM_COEF07                  0x0127
#define BBM_COEF08                  0x0128
#define BBM_COEF09                  0x0129
#define BBM_COEF0A                  0x012a
#define BBM_COEF0B                  0x012b
#define BBM_COEF0C                  0x012c
#define BBM_COEF0D                  0x012d
#define BBM_COEF0E                  0x012e
#define BBM_COEF0F                  0x012f
#define BBM_AGC530_EN               0x0132
#define BBM_BLOCK_AVG_SIZE_LOCK     0x0134
#define BBM_GAIN_CONSTANT           0x0138
#define BBM_DET_CNT_BOUND           0x013c
#define BBM_PGA_GAIN_MAX            0x0144
#define BBM_PGA_GAIN_MIN            0x0145
#define BBM_UNLOCK_DETECT_EN        0x0150

	/* SYNC */
#define BBM_OFDM_DET                0x0220
#define BBM_OFDM_DET_MAX_THRESHOLD  0x0222
#define BBM_DETECT_OFDM             0x0228
#define BBM_OFDM_DET_MODE_EN        0x022b
#define BBM_FTOFFSET_RANGE          0x0234
#define BBM_SYNC_MTH                0x0235
#define BBM_SYNC_CNTRL              0x0236
#define BBM_SYNC_STAT               0x0238
#define BBM_NCO_OFFSET              0x0278
#define BBM_NCO_INV                 0x027c
#define BBM_EZ_CONST                0x027d
#define BBM_CLK_MODE                0x027e
#define BBM_SFSYNC_ON               0x0280
#define BBM_INV_CARRIER_FREQ        0x0281
#define BBM_RESYNC_EN               0x02a0
#define BBM_RESYNC_AUTO_CONDITION_EN 0x02a1
#define BBM_RESYNC_CONDITION_INFO   0x02a2

	/* DIDP */
#define BBM_DIDP_MODE               0x0601
#define BBM_SCH0_SET_IDI            0x0602
#define BBM_SCH1_SET_IDI            0x0603
#define BBM_SCH2_SET_IDI            0x0604
#define BBM_PS0_RF_ENABLE           0x060a
#define BBM_PS0_RF_ADD_SHIFT        0x060b
#define BBM_PS1_ADC_ENABLE          0x060c
#define BBM_PS1_ADC_ADD_SHIFT       0x060d
#define BBM_PS2_BB_ENABLE           0x060e
#define BBM_PS2_BB_ADD_SHIFT        0x060f

	/* BUF */
#define BBM_BUF_STATUS              0x0900
#define BBM_BUF_OVERRUN             0x0902
#define BBM_BUF_ENABLE              0x0904
#define BBM_BUF_INT                 0x0906
#define BBM_BUF_STS_CH_ID           0x090a
#define BBM_CLK_EN                  0x090b
#define BBM_MISC_CTRL               0x090c
#define BBM_BUF_CH0_SUBID           0x0910
#define BBM_BUF_CH1_SUBID           0x0911
#define BBM_BUF_CH2_SUBID           0x0912
#define BBM_BUF_CH3_SUBID           0x0913
#define BBM_BUF_CH0_START           0x0918
#define BBM_BUF_CH1_START           0x091a
#define BBM_BUF_CH2_START           0x091c
#define BBM_BUF_CH3_START           0x091e
#define BBM_BUF_FIC_START           0x0928
#define BBM_BUF_CH0_END             0x0930
#define BBM_BUF_CH1_END             0x0932
#define BBM_BUF_CH2_END             0x0934
#define BBM_BUF_CH3_END             0x0936
#define BBM_BUF_FIC_END             0x0940
#define BBM_BUF_CH0_THR             0x0942
#define BBM_BUF_CH1_THR             0x0944
#define BBM_BUF_CH2_THR             0x0946
#define BBM_BUF_CH3_THR             0x0948
#define BBM_BUF_FIC_THR             0x0952

	/* I2C */
#define BBM_RF_PRER                 0x0a00
#define BBM_RF_CTR                  0x0a02
#define BBM_RF_RXR                  0x0a03
#define BBM_RF_SR                   0x0a04
#define BBM_RF_TXR                  0x0a05
#define BBM_RF_CR                   0x0a06
#define BBM_AP_FLAG                 0x0a07

	/* FEC */
#define BBM_FEC_RST                 0x0e00
#define BBM_FEC_ON                  0x0e02
#define BBM_FIC_CER_RXD_CRC         0x0e50
#define BBM_FIC_CER_ERR_CRC         0x0e52
#define BBM_FIC_CFG_CRC16           0x0e08
#define BBM_MSC_CFG_SCH0            0x0e0a
#define BBM_MSC_CFG_SCH1            0x0e0b
#define BBM_MSC_CFG_SPD             0x0e0e

	/* FIC I2C Read */
#define BBM_FIC_I2C_RD				0x7000

	/* DM */
#define BBM_DM                      0xf000

	/* BUFFER MANAGEMENT */
#define FIC_BUF_START   0x0000
#define FIC_BUF_LENGTH  (32 * 24)
#define FIC_BUF_END     (FIC_BUF_START + FIC_BUF_LENGTH - 1)
#define FIC_BUF_THR     (FIC_BUF_LENGTH / 2 - 1)

#define CH0_BUF_START   (FIC_BUF_START + FIC_BUF_LENGTH)
#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
#define CH0_BUF_LENGTH  (188*2)
#else
#define CH0_BUF_LENGTH  (188*40*2)
#endif
#define CH0_BUF_END     (CH0_BUF_START + CH0_BUF_LENGTH - 1)
#define CH0_BUF_THR     (CH0_BUF_LENGTH / 2 - 1)

#define CH1_BUF_START   (FIC_BUF_START + FIC_BUF_LENGTH)
#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
#define CH1_BUF_LENGTH  (188*2)
#else
#define CH1_BUF_LENGTH  (188*40*2)
#endif
#define CH1_BUF_END     (CH1_BUF_START + CH1_BUF_LENGTH - 1)
#define CH1_BUF_THR     (CH1_BUF_LENGTH / 2 - 1)

#define CH2_BUF_START   (FIC_BUF_START + FIC_BUF_LENGTH)
#define CH2_BUF_LENGTH  (128 * 6)
#define CH2_BUF_END     (CH2_BUF_START + CH2_BUF_LENGTH - 1)
#define CH2_BUF_THR     (CH2_BUF_LENGTH / 2 - 1)

#if (CH2_BUF_END >= 16384)
internal buffer is 16K, your setting value is big !!!!!!!!!!!!!!!
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_REGS_H__ */

