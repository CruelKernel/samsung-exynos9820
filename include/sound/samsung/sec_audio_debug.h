/*
 *  sec_audio_debug.c
 *
 *  Copyright (c) 2018 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _SEC_AUDIO_DEBUG_H
#define _SEC_AUDIO_DEBUG_H

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
int is_abox_rdma_enabled(int id);
int is_abox_wdma_enabled(int id);
void abox_debug_string_update(void);
#else
inline int is_abox_rdma_enabled(int)
{
	return 0;
}
inline int is_abox_wdma_enabled(int)
{
	return 0;
}
inline void abox_gpr_string_update(void)
{}
#endif

#endif
