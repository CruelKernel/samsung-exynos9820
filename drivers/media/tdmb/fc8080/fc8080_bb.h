/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_bb.h
 *
 *	Description : baseband header file
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

#ifndef __FC8080_BB_H__
#define __FC8080_BB_H__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8080_reset(HANDLE handle);
extern s32 fc8080_probe(HANDLE handle);
extern s32 fc8080_init(HANDLE handle);
extern s32 fc8080_deinit(HANDLE handle);
extern s32 fc8080_scan_status(HANDLE handle);
extern s32 fc8080_channel_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 fc8080_video_select(HANDLE handle, u8 subch_id, u8 buf_id,
				u8 cdi_id);
extern s32 fc8080_audio_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 fc8080_data_select(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 fc8080_channel_deselect(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 fc8080_video_deselect(HANDLE handle, u8 subch_id, u8 buf_id,
					u8 cdi_id);
extern s32 fc8080_audio_deselect(HANDLE handle, u8 subch_id, u8 buf_id);
extern s32 fc8080_data_deselect(HANDLE handle, u8 subch_id, u8 buf_id);

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_BB_H__ */

