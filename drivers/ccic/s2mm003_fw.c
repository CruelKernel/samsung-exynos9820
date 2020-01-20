/*
 * driver/../s2mm003.c - S2MM003 USBPD device driver
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <linux/ccic/s2mm003.h>
#include <linux/ccic/s2mm003_fw.h>

#if defined(CONFIG_SEC_CCIC_FW_FIX)
void s2mm003_firmware_ver_check(struct s2mm003_data *usbpd_data, char *name)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	usbpd_data->firm_ver[0] = (u8)s2mm003_indirect_read(usbpd_data, 0x01);
	usbpd_data->firm_ver[1] = (u8)s2mm003_indirect_read(usbpd_data, 0x02);
	usbpd_data->firm_ver[2] = (u8)s2mm003_indirect_read(usbpd_data, 0x03);
	usbpd_data->firm_ver[3] = (u8)s2mm003_indirect_read(usbpd_data, 0x04);
	dev_err(&i2c->dev, "%s %s version %02x %02x %02x %02x\n",
		USBPD_DEV_NAME, name, usbpd_data->firm_ver[0], usbpd_data->firm_ver[1],
		usbpd_data->firm_ver[2], usbpd_data->firm_ver[3]);
}
#endif

int s2mm003_firmware_update(struct s2mm003_data *usbpd_data)
{
	const struct firmware *fw_entry;
	const unsigned char *p;
	int ret, i;
	int fw_size = 0;
	const unsigned char *fw_start;
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 test, reg_check[3] = { 0, };

	u8 FW_CheckSum = 0;
	u8 Read_CheckSum = 0;

	ret = s2mm003_read_byte(i2c, I2C_SYSREG_SET, &reg_check[0]);
	ret = s2mm003_read_byte(i2c, I2C_SRAM_SET, &reg_check[1]);
	ret = s2mm003_read_byte(i2c, IF_S_CODE_E, &reg_check[2]);
	if ((reg_check[0] != 0) || (reg_check[1] != 0) || (reg_check[2] != 0))
	{
		dev_err(&usbpd_data->i2c->dev, "firmware register error %02x %02x %02x\n",
			reg_check[0], reg_check[1], reg_check[2]);
		s2mm003_write_byte(i2c, USB_PD_RST, 0x40);	/*FULL RESET*/
		msleep(20);
	}


	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x1);	/* 32 */
	s2mm003_read_byte_16(i2c, 0x1854, &test);
	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x0);	/* 32 */
	if (test > 7 || test < 0)
		dev_err(&usbpd_data->i2c->dev, "fw update err rid : %02x\n", test);

	ret = request_firmware(&fw_entry,
			       FIRMWARE_PATH, usbpd_data->dev);
	if (ret < 0)
		goto done;

	msleep(5);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, OTP_DUMP_DISABLE, 0x1); /* 30 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */
	msleep(10);

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x1);	/* 32 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x4);	/* sram update enable */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

/* update firmware */
	if(strncmp(fw_entry->data + 4,"\0\0\0\0\0\0\0\0\0\0\0\0", 12)) {
		pr_err("%s %d\n", __func__, __LINE__);
		fw_size = fw_entry->size;
		fw_start = fw_entry->data;

	} else {
		pr_err("%s %d\n", __func__, __LINE__);
		fw_size = (fw_entry->size) - sizeof(s2mm003_fw_header_t);
		fw_start = (fw_entry->data) + sizeof(s2mm003_fw_header_t);
	}
	for(p = fw_start, i=0; i < fw_size; p++, i++) {
		s2mm003_write_byte_16(i2c,(u16)i, *p);
		if (i >= 0x160)
			FW_CheckSum ^= *p;
	}
	if (fw_size % 16 != 0) {
		for(i=fw_size; i < ((fw_size / 16)+1)*16; i++) {
			s2mm003_write_byte_16(i2c,(u16)i, 0x00);
			FW_CheckSum ^= 0x00;
		}
	}

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x0);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x0);	/* 32 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, USB_PD_RST, 0x80);	/* 0D */
	s2mm003_write_byte(i2c, OTP_BLK_RST, 0x1);	/* 0C OTP controller
							   system register reset
							   to stop otp copy */
	msleep(5);
	s2mm003_write_byte(i2c, USB_PD_RST, 0x80);	/* 0D */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

	/* firmware checksum */
	msleep(5);
	Read_CheckSum = s2mm003_indirect_read(usbpd_data, 0x06);
	dev_info(&i2c->dev, "FW checksum read:0x%x \n", Read_CheckSum);

	if (FW_CheckSum == Read_CheckSum) {
		// FW check sum OK
		dev_info(&i2c->dev, "%s:CCIC FW checksum OK!!\n", __func__);
	} else {
		// for debugig ccic issue
		dev_info(&i2c->dev, "%s:CCIC FW checksum FAIL!!\n", __func__);
		//panic("CCIC FW checksum fail!!");
	}

	/* s2mm003 jig_on low timing issue  */
	s2mm003_indirect_write(usbpd_data, 0x13,1);
	s2mm003_indirect_write(usbpd_data, 0x13,3);

done:
	dev_err(&usbpd_data->i2c->dev, "firmware size: %d, error %d\n",
		(int)fw_entry->size, ret);
	if (fw_entry)
		release_firmware(fw_entry);
	return ret;
}
