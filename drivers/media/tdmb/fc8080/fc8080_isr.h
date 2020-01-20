/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_isr.h
 *
 *	Description : fc8080 interrupt service routine header file
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
 *
 *	History :
 *	----------------------------------------------------------------------
 */

#ifndef __FC8080_ISR__
#define __FC8080_ISR__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

extern u32 fic_user_data;
extern u32 msc_user_data;

extern s32 (*fic_callback)(u32 userdata, u8 *data, s32 length);
extern s32 (*msc_callback)(u32 userdata, u8 subch_id, u8 *data, s32 length);
extern void fc8080_isr(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FC8080_ISR__ */

