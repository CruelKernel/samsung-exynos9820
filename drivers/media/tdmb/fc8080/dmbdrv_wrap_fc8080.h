/*
 *	Copyright(c) 2008 SEC Corp. All Rights Reserved
 *
 *	File name : dmbdrv_wrap_fc8080.h
 *
 *	Description : fc8080 tuner control driver
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
 *	History :
 *	----------------------------------------------------------------------
 *
 */

#ifndef __DMBDRV_WRAP_FC8080_H
#define __DMBDRV_WRAP_FC8080_H

#include <linux/string.h>
#include <linux/delay.h>
#include "fc8080_regs.h"

#define TDMB_SUCCESS            1
#define TDMB_FAIL               0

#define USER_APPL_NUM_MAX       12
#define USER_APPL_DATA_SIZE_MAX 24

struct sub_channel_info_type {
	unsigned short uiEnsembleID;
	unsigned char	ucSubchID;
	unsigned short uiStartAddress;
	unsigned char ucTMId;
	unsigned char ucCAFlag;
	unsigned char ucServiceType;
	unsigned long ulServiceID;
	unsigned char num_of_user_appl;
	unsigned short user_appl_type[USER_APPL_NUM_MAX];
	unsigned char user_appl_length[USER_APPL_NUM_MAX];
	unsigned char user_appl_data[USER_APPL_NUM_MAX][USER_APPL_DATA_SIZE_MAX];
	unsigned char scids;
	unsigned char ecc;
};

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
void dmb_drv_isr(unsigned char *data, unsigned int length);
#else
void dmb_drv_isr(void);
#endif
unsigned char dmb_drv_init(unsigned long param
#ifdef CONFIG_TDMB_XTAL_FREQ
	, u32 xtal_freq
#endif
);
unsigned char dmb_drv_deinit(void);
unsigned char dmb_drv_scan_ch(unsigned long frequency);
int dmb_drv_get_dmb_sub_ch_cnt(void);
int dmb_drv_get_dab_sub_ch_cnt(void);
int dmb_drv_get_dat_sub_ch_cnt(void);
char *dmb_drv_get_ensemble_label(void);
char *dmb_drv_get_sub_ch_dmb_label(int subchannel_count);
char *dmb_drv_get_sub_ch_dab_label(int subchannel_count);
char *dmb_drv_get_sub_ch_dat_label(int subchannel_count);
struct sub_channel_info_type *dmb_drv_get_fic_dmb(int subchannel_count);
struct sub_channel_info_type *dmb_drv_get_fic_dab(int subchannel_count);
struct sub_channel_info_type *dmb_drv_get_fic_dat(int subchannel_count);
unsigned char dmb_drv_set_ch(
	unsigned long frequency
	, unsigned char subchannel
	, unsigned char sevice_type);
unsigned char dmb_drv_set_ch_factory(
	unsigned long frequency
	, unsigned char subchannel
	, unsigned char sevice_type);
unsigned short dmb_drv_get_ber(void);
unsigned char dmb_drv_get_ant(void);
signed short dmb_drv_get_rssi(void);
int tdmb_interrupt_fic_callback(u32 userdata, u8 *data, int length);
int tdmb_interrupt_msc_callback(
	u32 userdata, u8 subchannel_id, u8 *data, int length);
#endif
