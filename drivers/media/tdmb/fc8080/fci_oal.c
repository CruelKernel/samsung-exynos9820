/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fci_oal.c
 *
 *	Description : OS Adaptation Layer
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
#include <linux/delay.h>

#include "fci_types.h"

void print_log(HANDLE hDevice, char *fmt, ...)
{

}

void ms_wait(int ms)
{
	usleep_range(ms*1000, ms*1000);
}

