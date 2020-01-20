/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : bbm.h
 *
 *	Description : API header file of dmb baseband module
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
 *	History :
 *	----------------------------------------------------------------------
 *	20130422 v1.1.0
 *	20130613 v1.2.0
 *	20130704 v1.2.1
 *	20130710 v1.3.1
 *	20130716 v1.4.1
 *	20130806 v1.5.1
 *	20130926 v1.6.1
 *	20131125 v1.7.1
 *	20131202 v1.8.1
 */

#ifndef __BBM_H__
#define __BBM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

#define DRIVER_VER	"VER 1.8.1"

extern s32 bbm_com_reset(HANDLE handle);
extern s32 bbm_com_probe(HANDLE handle);
extern s32 bbm_com_init(HANDLE handle);
extern s32 bbm_com_deinit(HANDLE handle);
extern s32 bbm_com_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_com_byte_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_com_word_read(HANDLE handle, u16 addr, u16 *data);
extern s32 bbm_com_long_read(HANDLE handle, u16 addr, u32 *data);
extern s32 bbm_com_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 size);
extern s32 bbm_com_data(HANDLE handle, u16 addr, u8 *data, u32 size);
extern s32 bbm_com_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_com_byte_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_com_word_write(HANDLE handle, u16 addr, u16 data);
extern s32 bbm_com_long_write(HANDLE handle, u16 addr, u32 data);
extern s32 bbm_com_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 size);
extern s32 bbm_com_tuner_read(HANDLE handle, u8 addr, u8 addr_len,
				u8 *buffer, u8 len);
extern s32 bbm_com_tuner_write(HANDLE handle, u8 addr, u8 addr_len, u8 *buffer,
				u8 len);
extern s32 bbm_com_tuner_set_freq(HANDLE handle, u32 freq);
extern s32 bbm_com_tuner_select(HANDLE handle, u32 product, u32 band);
extern s32 bbm_com_tuner_get_rssi(HANDLE handle, s32 *rssi);
extern s32 bbm_com_scan_status(HANDLE handle);
extern s32 bbm_com_channel_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_video_select(HANDLE handle, u8 subch_id, u8 buf_id,
				u8 cdi_id);
extern s32 bbm_com_audio_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_data_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_channel_deselect(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_video_deselect(HANDLE handle, u8 subch_id, u8 buf_id,
					u8 cdi_id);
extern s32 bbm_com_audio_deselect(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_data_deselect(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 bbm_com_hostif_select(HANDLE handle, u8 hostif, unsigned long param);
extern s32 bbm_com_hostif_deselect(HANDLE handle);
extern s32 bbm_com_fic_callback_register(u32 userdata, s32 (*callback)(
					u32 userdata, u8 *data, s32 length));
extern s32 bbm_com_msc_callback_register(u32 userdata, s32 (*callback)(
					u32 userdata, u8 subch_id, u8 *data,
					s32 length));
extern s32 bbm_com_fic_callback_deregister(HANDLE handle);
extern s32 bbm_com_msc_callback_deregister(HANDLE handle);
extern void bbm_com_isr(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __BBM_H__ */

