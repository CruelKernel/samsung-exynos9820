/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : bbm.c
 *
 *	Description : API source file of dmb baseband module
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
 */

#include "fci_types.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8080_regs.h"
#include "fc8080_bb.h"
#include "fc8080_isr.h"

s32 bbm_com_reset(HANDLE handle)
{
	s32 res;

	res = fc8080_reset(handle);

	return res;
}

s32 bbm_com_probe(HANDLE handle)
{
	s32 res;

	res = fc8080_probe(handle);

	return res;
}

s32 bbm_com_init(HANDLE handle)
{
	s32 res;

	res = fc8080_init(handle);

	return res;
}

s32 bbm_com_deinit(HANDLE handle)
{
	s32 res;

	res = fc8080_deinit(handle);

	return res;
}

s32 bbm_com_read(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_read(handle, addr, data);

	return res;
}

s32 bbm_com_byte_read(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_byte_read(handle, addr, data);

	return res;
}

s32 bbm_com_word_read(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;

	res = bbm_word_read(handle, addr, data);

	return res;
}

s32 bbm_com_long_read(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;

	res = bbm_long_read(handle, addr, data);

	return res;
}

s32 bbm_com_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 size)
{
	s32 res;

	res = bbm_bulk_read(handle, addr, data, size);

	return res;
}

s32 bbm_com_data(HANDLE handle, u16 addr, u8 *data, u32 size)
{
	s32 res;

	res = bbm_data(handle, addr, data, size);

	return res;
}

s32 bbm_com_write(HANDLE handle, u16 addr, u8 data)
{
	s32 res;

	res = bbm_write(handle, addr, data);

	return res;
}

s32 bbm_com_byte_write(HANDLE handle, u16 addr, u8 data)
{
	s32 res;

	res = bbm_byte_write(handle, addr, data);

	return res;
}

s32 bbm_com_word_write(HANDLE handle, u16 addr, u16 data)
{
	s32 res;

	res = bbm_word_write(handle, addr, data);

	return res;
}

s32 bbm_com_long_write(HANDLE handle, u16 addr, u32 data)
{
	s32 res;

	res = bbm_long_write(handle, addr, data);

	return res;
}

s32 bbm_com_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 size)
{
	s32 res;

	res = bbm_bulk_write(handle, addr, data, size);

	return res;
}

s32 bbm_com_tuner_read(HANDLE handle, u8 addr, u8 addr_len, u8 *buffer,
			u8 len)
{
	s32 res;

	res = tuner_i2c_read(handle, addr, addr_len, buffer, len);

	return res;
}

s32 bbm_com_tuner_write(HANDLE handle, u8 addr, u8 addr_len, u8 *buffer, u8 len)
{
	s32 res;

	res = tuner_i2c_write(handle, addr, addr_len, buffer, len);

	return res;
}

s32 bbm_com_tuner_set_freq(HANDLE handle, u32 freq)
{
	s32 res;

	res = tuner_set_freq(handle, freq);

	return res;
}

s32 bbm_com_tuner_select(HANDLE handle, u32 product, u32 band)
{
	s32 res;

	res = tuner_select(handle, product, band);

	return res;
}

s32 bbm_com_tuner_get_rssi(HANDLE handle, s32 *rssi)
{
	s32 res;

	res = tuner_get_rssi(handle, rssi);

	return res;
}

s32 bbm_com_scan_status(HANDLE handle)
{
	s32 res;

	res = fc8080_scan_status(handle);

	return res;
}

s32 bbm_com_channel_select(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_channel_select(handle, subch_id, buf_id);

	return res;
}

s32 bbm_com_video_select(HANDLE handle, u8 subch_id, u8 buf_id, u8 cdi_id)
{
	s32 res;

	res = fc8080_video_select(handle, subch_id, buf_id, cdi_id);

	return res;
}

s32 bbm_com_audio_select(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_audio_select(handle, subch_id, buf_id);

	return res;
}

s32 bbm_com_data_select(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_data_select(handle, subch_id, buf_id);

	return res;
}

s32 bbm_com_channel_deselect(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_channel_deselect(handle, subch_id, buf_id);

	return res;
}

s32 bbm_com_video_deselect(HANDLE handle, u8 subch_id, u8 buf_id, u8 cdi_id)
{
	s32 res;

	res = fc8080_video_deselect(handle, subch_id, buf_id, cdi_id);

	return res;
}

s32 bbm_com_audio_deselect(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_audio_deselect(handle, subch_id, buf_id);

	return res;
}

s32 bbm_com_data_deselect(HANDLE handle, u8 subch_id, u8 buf_id)
{
	s32 res;

	res = fc8080_data_deselect(handle, subch_id, buf_id);

	return res;
}

void bbm_com_isr(HANDLE handle)
{
	fc8080_isr(handle);
}

s32 bbm_com_hostif_select(HANDLE handle, u8 hostif, unsigned long param)
{
	s32 res;

	res = bbm_hostif_select(handle, hostif, param);

	return res;
}

s32 bbm_com_hostif_deselect(HANDLE handle)
{
	s32 res;

	res = bbm_hostif_deselect(handle);

	return res;
}

s32 bbm_com_fic_callback_register(u32 userdata, s32 (*callback)(u32 userdata,
					u8 *data, s32 length))
{
	fic_user_data = userdata;
	fic_callback = callback;

	return BBM_OK;
}

s32 bbm_com_msc_callback_register(u32 userdata, s32 (*callback)(u32 userdata,
					u8 subch_id, u8 *data, s32 length))
{
	msc_user_data = userdata;
	msc_callback = callback;

	return BBM_OK;
}

s32 bbm_com_fic_callback_deregister(HANDLE handle)
{
	fic_user_data = 0;
	fic_callback = NULL;

	return BBM_OK;
}

s32 bbm_com_msc_callback_deregister(HANDLE handle)
{
	msc_user_data = 0;
	msc_callback = NULL;

	return BBM_OK;
}

