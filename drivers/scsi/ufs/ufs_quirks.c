/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ufshcd.h"
#include "ufs_quirks.h"

#define SERIAL_NUM_SIZE 7
#define TOSHIBA_SERIAL_NUM_SIZE 10

void ufs_set_sec_unique_number(struct ufs_hba *hba, u8 *str_desc_buf, u8 *desc_buf)
{
	u8 manid;
	u8 snum_buf[UFS_UN_MAX_DIGITS];

	manid = hba->manufacturer_id & 0xFF;
	memset(hba->unique_number, 0, sizeof(hba->unique_number));
	memset(snum_buf, 0, sizeof(snum_buf));

	memcpy(snum_buf, str_desc_buf + QUERY_DESC_HDR_SIZE, SERIAL_NUM_SIZE);

	sprintf(hba->unique_number, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		manid,
		desc_buf[DEVICE_DESC_PARAM_MANF_DATE], desc_buf[DEVICE_DESC_PARAM_MANF_DATE+1],
		snum_buf[0], snum_buf[1], snum_buf[2], snum_buf[3], snum_buf[4], snum_buf[5], snum_buf[6]);

	/* Null terminate the unique number string */
	hba->unique_number[UFS_UN_20_DIGITS] = '\0';
}
