/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_hal.h
 *
 *	Description : fc8080 host interface header file
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

#ifndef __FCI_HAL_H__
#define __FCI_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 bbm_hostif_select(HANDLE handle, u8 hostif, unsigned long param);
extern s32 bbm_hostif_deselect(HANDLE handle);
extern s32 bbm_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_byte_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_word_read(HANDLE handle, u16 addr, u16 *data);
extern s32 bbm_long_read(HANDLE handle, u16 addr, u32 *data);
extern s32 bbm_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 bbm_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_byte_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_word_write(HANDLE handle, u16 addr, u16 data);
extern s32 bbm_long_write(HANDLE handle, u16 addr, u32 data);
extern s32 bbm_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 bbm_data(HANDLE handle, u16 addr, u8 *data, u32 length);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_HAL_H__ */

