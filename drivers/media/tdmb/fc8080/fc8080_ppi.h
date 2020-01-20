/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_ppi.h
 *
 *	Description : EBI2LCD interface header file
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

#ifndef __FC8080_PPI_H__
#define __FC8080_PPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_TDMB_EBI)
extern void *v_addr_ebi_cs_base;
#endif

extern s32 fc8080_ppi_init(HANDLE handle, u16 param1, u16 param2);
extern s32 fc8080_ppi_byteread(HANDLE handle, u16 addr, u8 *data);
extern s32 fc8080_ppi_wordread(HANDLE handle, u16 addr, u16 *data);
extern s32 fc8080_ppi_longread(HANDLE handle, u16 addr, u32 *data);
extern s32 fc8080_ppi_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8080_ppi_bytewrite(HANDLE handle, u16 addr, u8 data);
extern s32 fc8080_ppi_wordwrite(HANDLE handle, u16 addr, u16 data);
extern s32 fc8080_ppi_longwrite(HANDLE handle, u16 addr, u32 data);
extern s32 fc8080_ppi_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8080_ppi_dataread(HANDLE handle, u16 addr, u8 *data, u32 length);
extern s32 fc8080_ppi_deinit(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_PPI_H__ */

