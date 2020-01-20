/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fci_hal.c
 *
 *	Description : fc8080 host interface source file
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
#include "fc8080_i2c.h"
#if defined(CONFIG_TDMB_SPI)
#include "fc8080_spi.h"
#endif
#if defined(CONFIG_TDMB_EBI)
#include "fc8080_ppi.h"
#endif

struct interface_port {
	s32 (*init)(HANDLE handle, unsigned long param);
	s32 (*byteread)(HANDLE handle, u16 addr, u8  *data);
	s32 (*wordread)(HANDLE handle, u16 addr, u16 *data);
	s32 (*longread)(HANDLE handle, u16 addr, u32 *data);
	s32 (*bulkread)(HANDLE handle, u16 addr, u8  *data, u16 size);
	s32 (*bytewrite)(HANDLE handle, u16 addr, u8  data);
	s32 (*wordwrite)(HANDLE handle, u16 addr, u16 data);
	s32 (*longwrite)(HANDLE handle, u16 addr, u32 data);
	s32 (*bulkwrite)(HANDLE handle, u16 addr, u8 *data, u16 size);
	s32 (*dataread)(HANDLE handle, u16 addr, u8 *data, u32 size);
	s32 (*deinit)(HANDLE handle);
};

#if defined(CONFIG_TDMB_SPI)
static struct interface_port spiif = {
	&fc8080_spi_init,
	&fc8080_spi_byteread,
	&fc8080_spi_wordread,
	&fc8080_spi_longread,
	&fc8080_spi_bulkread,
	&fc8080_spi_bytewrite,
	&fc8080_spi_wordwrite,
	&fc8080_spi_longwrite,
	&fc8080_spi_bulkwrite,
	&fc8080_spi_dataread,
	&fc8080_spi_deinit
};
#endif
#if defined(CONFIG_TDMB_EBI)
static struct interface_port ppiif = {
	&fc8080_ppi_init,
	&fc8080_ppi_byteread,
	&fc8080_ppi_wordread,
	&fc8080_ppi_longread,
	&fc8080_ppi_bulkread,
	&fc8080_ppi_bytewrite,
	&fc8080_ppi_wordwrite,
	&fc8080_ppi_longwrite,
	&fc8080_ppi_bulkwrite,
	&fc8080_ppi_dataread,
	&fc8080_ppi_deinit
};
#endif
#if defined(CONFIG_TDMB_I2C)
static struct interface_port i2cif = {
	&fc8080_i2c_init,
	&fc8080_i2c_byteread,
	&fc8080_i2c_wordread,
	&fc8080_i2c_longread,
	&fc8080_i2c_bulkread,
	&fc8080_i2c_bytewrite,
	&fc8080_i2c_wordwrite,
	&fc8080_i2c_longwrite,
	&fc8080_i2c_bulkwrite,
	&fc8080_i2c_dataread,
	&fc8080_i2c_deinit
};
#endif

static struct interface_port *ifport;
static u8 hostif_type;

s32 bbm_hostif_select(HANDLE handle, u8 hostif, unsigned long param)
{
	hostif_type = hostif;

	switch (hostif) {
#if defined(CONFIG_TDMB_SPI)
	case BBM_SPI:
		ifport = &spiif;
		break;
#endif
#if defined(CONFIG_TDMB_I2C)
	case BBM_I2C:
		ifport = &i2cif;
		break;
#endif
#if defined(CONFIG_TDMB_EBI)
	case BBM_PPI:
		ifport = &ppiif;
		break;
#endif
	default:
		return BBM_E_HOSTIF_SELECT;
	}

	if (ifport->init(handle, param))
		return BBM_E_HOSTIF_INIT;

	return BBM_OK;
}

s32 bbm_hostif_deselect(HANDLE handle)
{
	if (ifport->deinit(handle))
		return BBM_NOK;

	hostif_type = 0;

	return BBM_OK;
}

s32 bbm_read(HANDLE handle, u16 addr, u8 *data)
{
	if (ifport->byteread(handle, addr, data))
		return BBM_E_BB_READ;

	return BBM_OK;
}

s32 bbm_byte_read(HANDLE handle, u16 addr, u8 *data)
{
	if (ifport->byteread(handle, addr, data))
		return BBM_E_BB_READ;

	return BBM_OK;
}

s32 bbm_word_read(HANDLE handle, u16 addr, u16 *data)
{
	if (ifport->wordread(handle, addr, data))
		return BBM_E_BB_READ;

	return BBM_OK;
}

s32 bbm_long_read(HANDLE handle, u16 addr, u32 *data)
{
	if (ifport->longread(handle, addr, data))
		return BBM_E_BB_READ;

	return BBM_OK;
}

s32 bbm_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkread(handle, addr, data, length))
		return BBM_E_BB_READ;

	return BBM_OK;
}

s32 bbm_write(HANDLE handle, u16 addr, u8 data)
{
	if (ifport->bytewrite(handle, addr, data))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

s32 bbm_byte_write(HANDLE handle, u16 addr, u8 data)
{
	if (ifport->bytewrite(handle, addr, data))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

s32 bbm_word_write(HANDLE handle, u16 addr, u16 data)
{
	if (ifport->wordwrite(handle, addr, data))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

s32 bbm_long_write(HANDLE handle, u16 addr, u32 data)
{
	if (ifport->longwrite(handle, addr, data))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

s32 bbm_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkwrite(handle, addr, data, length))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

s32 bbm_data(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	if (ifport->dataread(handle, addr, data, length))
		return BBM_E_BB_WRITE;

	return BBM_OK;
}

