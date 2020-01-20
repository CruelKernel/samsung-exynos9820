/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_isr.c
 *
 *	Description : fc8080 interrupt service routine source file
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
#include <linux/kernel.h>
#include <linux/cache.h>
#include "fci_types.h"
#include "fci_hal.h"
#include "fc8080_regs.h"
#include "fc8080_isr.h"
#include "tdmb.h"

#define FC8080_OVERRUN_CLEAR

static u8 fic_buffer[512+4] __cacheline_aligned;
static u8 msc_buffer[8192+4] __cacheline_aligned;

s32 (*fic_callback)(u32 userdata, u8 *data, s32 length) = NULL;
s32 (*msc_callback)(u32 userdata, u8 subch_id, u8 *data, s32 length) = NULL;

u32 fic_user_data;
u32 msc_user_data;

static void fc8080_data(HANDLE handle, u16 status)
{
	u16 size;
	s32 i;

	if (status & 0x0100) {
		bbm_word_read(handle, BBM_BUF_FIC_THR, &size);
		size++;

		bbm_data(handle, BBM_RD_FIC, &fic_buffer[0], size);
#ifdef CONFIG_TDMB_SPI
		if (fic_callback)
			(*fic_callback)(fic_user_data, &fic_buffer[4], size);
#else
		if (fic_callback)
			(*fic_callback)(fic_user_data, &fic_buffer[0], size);
#endif
	}

	for (i = 0; i < 3; i++) {
		u8 subch_id;

		if (!(status & (1 << i)))
			continue;
		bbm_word_read(handle, BBM_BUF_CH0_THR + i * 2, &size);
		size++;
		bbm_read(handle, BBM_BUF_CH0_SUBID + i, &subch_id);
		subch_id &= 0x3f;
		bbm_data(handle, (BBM_RD_BUF0 + i), &msc_buffer[0], size);
#ifdef CONFIG_TDMB_SPI
		if (msc_callback)
			(*msc_callback)(msc_user_data, subch_id, &msc_buffer[4],
						size);
#else
		if (msc_callback)
			(*msc_callback)(msc_user_data, subch_id, &msc_buffer[0],
						size);
#endif
	}
}

void fc8080_isr(HANDLE handle)
{
	u16 buf_int_status = 0;
	u16 buf_ovr_status;

	bbm_word_read(handle, BBM_BUF_STATUS, &buf_int_status);
	bbm_word_read(handle, BBM_BUF_OVERRUN, &buf_ovr_status);
	if (buf_int_status) {
		bbm_word_write(handle, BBM_BUF_STATUS, buf_int_status);
		fc8080_data(handle, buf_int_status);

		if (buf_ovr_status) {
			bbm_word_write(handle, BBM_BUF_OVERRUN, buf_ovr_status);
			bbm_word_write(handle, BBM_BUF_OVERRUN, 0);
			DPRINTK("[FC8080] Overrun clear\n");
		}
	}
#ifdef FC8080_OVERRUN_CLEAR
	else {
		if (buf_ovr_status) {
			bbm_word_write(handle, BBM_BUF_OVERRUN, buf_ovr_status);
			bbm_word_write(handle, BBM_BUF_OVERRUN, 0);
			DPRINTK("[FC8080] Overrun occurred : 0x%x\n"
				, buf_ovr_status);
			fc8080_data(handle, buf_ovr_status);
		}
	}
#endif
}
