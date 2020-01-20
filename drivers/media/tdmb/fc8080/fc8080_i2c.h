/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_i2c.h
 *
 *	Description : i2c interface header file
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

#ifndef __FC8080_I2C_H__
#define __FC8080_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8080_i2c_init(HANDLE handle, unsigned long param);
extern s32 fc8080_i2c_byteread(HANDLE handle, u16 addr, u8 *data);
extern s32 fc8080_i2c_wordread(HANDLE handle, u16 addr, u16 *data);
extern s32 fc8080_i2c_longread(HANDLE handle, u16 addr, u32 *data);
extern s32 fc8080_i2c_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8080_i2c_bytewrite(HANDLE handle, u16 addr, u8 data);
extern s32 fc8080_i2c_wordwrite(HANDLE handle, u16 addr, u16 data);
extern s32 fc8080_i2c_longwrite(HANDLE handle, u16 addr, u32 data);
extern s32 fc8080_i2c_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8080_i2c_dataread(HANDLE handle, u16 addr, u8 *data, u32 length);
extern s32 fc8080_i2c_deinit(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_I2C_H__ */

