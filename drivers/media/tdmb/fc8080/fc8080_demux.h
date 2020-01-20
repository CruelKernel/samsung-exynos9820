/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_demux.h
 *
 *	Description : fc8080 TSIF demux header file
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

#ifndef __FC8080_DEMUX_H__
#define __FC8080_DEMUX_H__

#ifdef __cplusplus
extern "C" {
#endif

int fc8080_demux(u8 *data, u32 length);
int fc8080_demux_fic_callback_register(u32 userdata,
	int (*callback)(u32 userdata, u8 *data, int length));
int fc8080_demux_msc_callback_register(u32 userdata,
	int (*callback)(u32 userdata, u8 subch_id, u8 *data, int length));
int fc8080_demux_select_video(u8 subch_id, u8 buf_id);
int fc8080_demux_select_channel(u8 subch_id, u8 buf_id);
int fc8080_demux_deselect_video(u8 subch_id, u8 buf_id);
int fc8080_demux_deselect_channel(u8 subch_id, u8 buf_id);

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_DEMUX_H__ */
