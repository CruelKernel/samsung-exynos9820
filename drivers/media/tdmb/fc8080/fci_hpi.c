/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fci_hpi.c
 *
 *	Description : tuner interface source file
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

#include "fci_types.h"
#include "fci_hal.h"
#include "fc8080_regs.h"

s32 fci_hpi_init(HANDLE handle, s32 speed, s32 slaveaddr)
{
	return BBM_OK;
}

s32 fci_hpi_read(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	return bbm_bulk_read(handle, 0x0f00 | addr, data, len);
}

s32 fci_hpi_write(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
#ifdef FC8080_SPI
	s32 i;

	for (i = 0; i < len; i++, data++)
		bbm_word_write(handle, 0x0f00 | addr, (*data << 8) | *data);
#else
	s32 i;

	for (i = 0; i < len; i++, data++)
		bbm_write(handle, 0x0f00 | addr, *data);
	/*return bbm_bulk_write(handle, 0x0f00 | addr, data, len);*/
#endif

	return BBM_OK;
}

s32 fci_hpi_deinit(HANDLE handle)
{
	return BBM_OK;
}

