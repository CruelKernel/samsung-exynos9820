#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ccic/s2mm005.h>
#include <linux/ccic/s2mm005_ext.h>
#include <linux/ccic/s2mm005_fw.h>
#include <linux/ccic/ccic_sysfs.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT5.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT6.h>
#include <linux/ccic/BOOT_FLASH_FW_0x0A_BOOT7.h>
#include <linux/ccic/BOOT_FLASH_FW_0x01_BOOT7.h>
#include <linux/ccic/BOOT_SRAM_FW.h>

#define	S2MM005_FIRMWARE_PATH	"usbpd/s2mm005.bin"

#define FW_CHECK_RETRY 5
#define VALID_FW_BOOT_VERSION(fw_boot) (fw_boot == 0x7)
#define VALID_FW_MAIN_VERSION(fw_main) \
	(!((fw_main[0] == 0xff) && (fw_main[1] == 0xff)) \
 	&& !((fw_main[0] == 0x00) && (fw_main[1] == 0x00)))

const char *flashmode_to_string(u32 mode)
{
	switch (mode) {
#define FLASH_MODE_STR(mode) case mode: return(#mode)
	FLASH_MODE_STR(FLASH_MODE_NORMAL);
	FLASH_MODE_STR(FLASH_MODE_FLASH);
	FLASH_MODE_STR(FLASH_MODE_ERASE);
	}
	return "?";
}

int s2mm005_sram_write(const struct i2c_client *i2c)
{
	int ret = 0;
	struct i2c_msg msg[1];
	struct s2mm005_data *usbpd_data = i2c_get_clientdata(i2c);

	pr_err("%s size:%d\n", __func__, BOOT_SRAM_FW_SIZE);
	mutex_lock(&usbpd_data->i2c_mutex);
	msg[0].addr = 0x3B;	/* Slave addr 0x76 */
	msg[0].flags = 0;
	msg[0].len = BOOT_SRAM_FW_SIZE;
	msg[0].buf = (u8 *)&BOOT_SRAM_FW[0];

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail error %d\n", ret);
	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

void s2mm005_write_flash(const struct i2c_client *i2c,
			 unsigned int fAddr, unsigned int fData) {
	u8 data[8];

	data[0] = 0x42;
	data[1] = 0x04; /* long write */

	data[2] = (u8)(fAddr & 0xFF);
	data[3] = (u8)((fAddr >> 8) & 0xFF);

	data[4] = (uint8_t)(fData & 0xFF);
	data[5] = (uint8_t)((fData>>8) & 0xFF);

	data[6] = (uint8_t)((fData>>16) & 0xFF);
	data[7] = (uint8_t)((fData>>24) & 0xFF);
/*	pr_info("Flash Write Address :0x%08X Data:0x%08X\n", fAddr, fData);*/
	s2mm005_write_byte(i2c, 0x10, &data[0], 8);
}

void s2mm005_verify_flash(const struct i2c_client *i2c,
			  uint32_t fAddr, uint32_t *fData) {
	uint16_t REG_ADD;
	uint8_t W_DATA[8];
	uint8_t R_DATA[4];

	uint32_t fRead[3];

	uint32_t fCnt;

	for (fCnt = 0; fCnt < 2; fCnt++) {
		W_DATA[0] = 0x42;
		W_DATA[1] = 0x40;  /* Long Read */

		W_DATA[2] = (uint8_t)(fAddr & 0xFF);
		W_DATA[3] = (uint8_t)((fAddr>>8) & 0xFF);


		REG_ADD = 0x10;
		udelay(10);
		s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 4);
		REG_ADD = 0x14;
		udelay(10);
		s2mm005_read_byte_flash(i2c, REG_ADD, &R_DATA[0], 4);

		fRead[fCnt] = 0;

		fRead[fCnt] |= R_DATA[0];
		fRead[fCnt] |= (R_DATA[1]<<8);
		fRead[fCnt] |= (R_DATA[2]<<16);
		fRead[fCnt] |= (R_DATA[3]<<24);

	}

	if (fRead[0] == fRead[1]) {
		*fData = fRead[0];
		/* pr_err("> Flash Read   Address : 0x%08X    Data : 0x%08X\n",fAddr, *fData); */
	} else {
		W_DATA[0] = 0x42;
		W_DATA[1] = 0x40;  /* Long Read */

		W_DATA[2] = (uint8_t)(fAddr & 0xFF);
		W_DATA[3] = (uint8_t)((fAddr>>8) & 0xFF);


		REG_ADD = 0x10;
		udelay(10);
		s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 4);
		REG_ADD = 0x14;
		udelay(10);
		s2mm005_read_byte_flash(i2c, REG_ADD, &R_DATA[0], 4);

		fRead[fCnt] = 0;

		fRead[2] |= R_DATA[0];
		fRead[2] |= (R_DATA[1]<<8);
		fRead[2] |= (R_DATA[2]<<16);
		fRead[2] |= (R_DATA[3]<<24);

		if (fRead[2] == fRead[0]) {
			*fData = fRead[0];
			pr_err("> Flash Read[0]   Address : 0x%08X    Data : 0x%08X\n", fAddr, *fData);
		} else if (fRead[2] == fRead[1]) {
			*fData = fRead[1];
			pr_err("> Flash Read[1]   Address : 0x%08X    Data : 0x%08X\n", fAddr, *fData);
		} else {
			*fData = 0;
			pr_err("> Flash Read[FAIL]   Address : 0x%08X    Data : 0x%08X\n", fAddr, *fData);
		}
	}
}

static int s2mm005_flash_write(struct s2mm005_data *usbpd_data, unsigned char *fw_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg, fError;
	uint32_t *pFlash_FW, *pFlash_FWCS;
	uint32_t LopCnt, fAddr, fData, fRData, sLopCnt;
	uint32_t recheck_count = 0;
	struct s2mm005_fw *fw_hd;
	unsigned int size;

	reg = FLASH_WRITE_0x42;
	s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
	reg = FLASH_WRITING_BYTE_SIZE_0x4;
	s2mm005_write_byte(i2c, CMD_HOST_0x11, &reg, 1);
	s2mm005_read_byte_flash(i2c, FLASH_STATUS_0x24, &val, 1);

	pFlash_FW = (uint32_t *)(fw_data + 12);
	pFlash_FWCS = (uint32_t *)fw_data;
	fw_hd = (struct s2mm005_fw*)fw_data;
	size = fw_hd -> size;
	if(fw_hd -> boot < 6)
    	sLopCnt = 0x1000/4;
 	else if (fw_hd -> boot == 6)
        sLopCnt = 0x8000/4;
 	else if (fw_hd -> boot >= 7)
     	sLopCnt = 0x7000/4; 

	/* Flash write */
	for (LopCnt = sLopCnt; LopCnt < (size/4); LopCnt++) {
		fAddr = LopCnt*4;
		fData = pFlash_FW[LopCnt];
		udelay(10);
		s2mm005_write_flash(i2c, fAddr, fData);
	}
	usleep_range(10 * 1000, 10 * 1000);
	/* Partial verify */
	while(1) {
		for (LopCnt = sLopCnt; LopCnt < (size/4); LopCnt++) {
			fError = 1;
			fAddr = LopCnt*4;
			fData = pFlash_FW[LopCnt];
			s2mm005_verify_flash(i2c, fAddr, &fRData);
			if (fData != fRData) {
				recheck_count++;
				LopCnt = (fAddr & 0xffffff00) / 4;
				sLopCnt = LopCnt;
				fError = 0;
				pr_err("%s partial verify fail!! recheck count : %d\n", __func__, recheck_count);
				pr_err("Verify Error Address = 0x%08X	 WData = 0x%08X    VData = 0x%08X\n", fAddr, fData, fRData);
				s2mm005_write_flash(i2c, fAddr, fData);
				msleep(20);
				if(recheck_count == 1000)
					return -EFLASH_VERIFY;
				break;
			}
		}
		if(fError)
			break;
	}
	pr_err("%s verify success!! recheck count : %d\n", __func__, recheck_count);

	if (LopCnt >= (size / 4)) {
		if (fw_hd -> boot >= 6) {
			/* SW version from header */
			recheck_count = 0;
			while(1) {
				recheck_count++;
				fAddr = 0xEFF0;
				pr_err("SW version = 0x%08X\n", pFlash_FWCS[0]);
				fData = pFlash_FWCS[0];
				s2mm005_write_flash(i2c, fAddr, fData);
				fRData = 0;
				s2mm005_verify_flash(i2c, fAddr, &fRData);
				if(fData == fRData)
					break;
				else {
					if (recheck_count == 30)
						return -EFLASH_WRITE_SWVERSION;
				}
			}
			/* Size from header */
			recheck_count = 0;
			while(1) {
				recheck_count++;
				fAddr = 0xEFF4;
				pr_err("SW Size = 0x%08X\n", pFlash_FWCS[2]);
				fData = pFlash_FWCS[2];
				s2mm005_write_flash(i2c, fAddr, fData);
				fRData = 0;
				s2mm005_verify_flash(i2c, fAddr, &fRData);
				if(fData == fRData)
					break;
				else {
					if (recheck_count == 30)
						return -EFLASH_WRITE_SIZE;
				}
			}
			/* CRC Check sum */
			recheck_count = 0;
			while(1) {
				recheck_count++;
				fAddr = 0xEFF8;
				pr_err("SW CheckSum = 0x%08X\n", pFlash_FWCS[((size + 16) / 4) - 1]);
				fData = pFlash_FWCS[((size + 16) / 4) - 1];
				s2mm005_write_flash(i2c, fAddr, fData);
				fRData = 0;
				s2mm005_verify_flash(i2c, fAddr, &fRData);
				if(fData == fRData)
					break;
				else {
					if (recheck_count == 30)
						return -EFLASH_WRITE_CRC;
				}
			}
		}

		/* flash done */
		recheck_count = 0;
		while(1)
		{
			recheck_count++;
			fAddr = 0xEFFC;
			fData = 0x1;
			s2mm005_write_flash(i2c, fAddr, fData);
			fRData = 0;
			s2mm005_verify_flash(i2c, fAddr, &fRData);
			pr_err("0xeffc = %d\n", fRData);
			if(fData == fRData)
				break;
			else {
				if (recheck_count == 30)
					return -EFLASH_WRITE_DONE;
			}
		}
		pr_err("%s flash write succesfully done!!\n", __func__);
	}

	return 0;
}

void s2mm005_flash_ready(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 W_DATA[5];

	/* FLASH_READY */
	W_DATA[0] = 0x02;
	W_DATA[1] = 0x01;
	W_DATA[2] = 0x30;
	W_DATA[3] = 0x50;
	W_DATA[4] = 0x01;
	s2mm005_write_byte(i2c, CMD_MODE_0x10, &W_DATA[0], 5);
}

int s2mm005_flash(struct s2mm005_data *usbpd_data, unsigned int input)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg;
	int ret = 0;
	int retry = 0;
	struct s2mm005_fw *fw_hd;
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int irq_gpio_status;
	FLASH_STATE_Type Flash_DATA;

	switch (input) {
	case FLASH_MODE_ENTER: { /* enter flash mode */
		/* FLASH_READY */
		s2mm005_flash_ready(usbpd_data);
		do {
			/* FLASH_MODE */
			reg = FLASH_MODE_ENTER_0x10;
			s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
			usleep_range(50 * 1000, 50 * 1000);
			/* If irq status is not clear, CCIC can not enter flash mode. */
			irq_gpio_status = gpio_get_value(usbpd_data->irq_gpio);
			dev_info(&i2c->dev, "%s IRQ0:%02d\n", __func__, irq_gpio_status);
			if(!irq_gpio_status) {
				s2mm005_int_clear(usbpd_data);	// interrupt clear
				usleep_range(10 * 1000, 10 * 1000);
			}
			s2mm005_read_byte_flash(i2c, FLASH_STATUS_0x24, &val, 1);
			pr_err("%s %s retry %d\n", __func__, flashmode_to_string(val), retry);
			usleep_range(50*1000, 50*1000);

			s2mm005_read_byte(i2c, 0x24, Flash_DATA.BYTE, 4);
			dev_info(&i2c->dev, "Flash_State:0x%02X   Reserved:0x%06X\n",
				Flash_DATA.BITS.Flash_State, Flash_DATA.BITS.Reserved);

			if(val != FLASH_MODE_FLASH) {
				retry++;
				if(retry == 10) {
					/* RESET */
					s2mm005_reset(usbpd_data);
					msleep(3000);
					/* FLASH_READY */
					s2mm005_flash_ready(usbpd_data);
				} else if (retry == 20) {
					panic("Flash mode change fail!\n");
				}
			}
		} while (val != FLASH_MODE_FLASH);
		break;
	}
	case FLASH_ERASE: { /* erase flash */
		reg = FLASH_ERASE_0x44;
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
		msleep(200);
		s2mm005_read_byte_flash(i2c, FLASH_STATUS_0x24, &val, 1);
		pr_err("flash mode : %s\n", flashmode_to_string(val));
		break;
	}
	case FLASH_WRITE5: { /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char*)&BOOT_FLASH_FW_BOOT5[0]);
		break;
	}
	case FLASH_WRITE6: { /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char*)&BOOT_FLASH_FW_BOOT6[0]);
		break;
	}
	case FLASH_WRITE7: { /* write flash & verify */
		switch (usbpd_data->s2mm005_fw_product_id) {
			case PRODUCT_NUM_GREAT:
				ret = s2mm005_flash_write(usbpd_data, (unsigned char*)&BOOT_FLASH_FW_0x0A_BOOT7[0]);
				break;
			case PRODUCT_NUM_DREAM:
			default:
				ret = s2mm005_flash_write(usbpd_data, (unsigned char*)&BOOT_FLASH_FW_0x01_BOOT7[0]);
				break;
		}
		break;
	}
	case FLASH_WRITE_UMS: {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		fp = filp_open(CCIC_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
		if (IS_ERR(fp)) {
			pr_err("%s: failed to open %s.\n", __func__,
				      CCIC_DEFAULT_UMS_FW);
			ret = -ENOENT;
			set_fs(old_fs);
			return ret;
		}

		fw_size = fp->f_path.dentry->d_inode->i_size;
		if (0 < fw_size) {
			unsigned char *fw_data;
			fw_data = kzalloc(fw_size, GFP_KERNEL);
			nread = vfs_read(fp, (char __user *)fw_data,
					 fw_size, &fp->f_pos);

			pr_err("%s: start, file path %s, size %ld Bytes\n",
				       __func__, CCIC_DEFAULT_UMS_FW, fw_size);

			if (nread != fw_size) {
				pr_err("%s: failed to read firmware file, nread %ld Bytes\n",
					      __func__, nread);
				ret = -EIO;
			} else {
				fw_hd = (struct s2mm005_fw*)fw_data;
				pr_err("%02X %02X %02X %02X size:%05d\n", fw_hd->boot, fw_hd->main[0], fw_hd->main[1], fw_hd->main[2], fw_hd->size);
				/* TODO : DISABLE IRQ */
				/* TODO : FW UPDATE */
				ret = s2mm005_flash_write(usbpd_data, (unsigned char*)fw_data);
				/* TODO : ENABLE IRQ */
			}
			kfree(fw_data);
		}

		filp_close(fp, NULL);
		set_fs(old_fs);
		break;
	}
	case FLASH_MODE_EXIT: { /* exit flash mode */
		reg = FLASH_MODE_EXIT_0x20;
		s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
		usleep_range(15 * 1000, 15 * 1000);
		s2mm005_read_byte_flash(i2c, FLASH_STATUS_0x24, &val, 1);
		pr_err("flash mode : %s\n", flashmode_to_string(val));
		break;
	}
	default: {
		pr_err("Flash value does not matched menu\n");

	}
	}
	return ret;
}

void s2mm005_get_fw_version(int product_id,
	struct s2mm005_version *version, u8 boot_version, u32 hw_rev)
{
	struct s2mm005_fw *fw_hd;
	switch (boot_version) {
	case 5:
		fw_hd = (struct s2mm005_fw*) BOOT_FLASH_FW_BOOT5;
		break;
	case 6:
		fw_hd = (struct s2mm005_fw*) BOOT_FLASH_FW_BOOT6;
		break;
	case 7:
	default:
		switch (product_id) {
			case PRODUCT_NUM_GREAT:
				fw_hd = (struct s2mm005_fw*) BOOT_FLASH_FW_0x0A_BOOT7;
				break;
			case PRODUCT_NUM_DREAM:
			default:
				fw_hd = (struct s2mm005_fw*) BOOT_FLASH_FW_0x01_BOOT7;
				break;
		}
		break;
	}
	version->boot = fw_hd->boot;
	version->main[0] = fw_hd->main[0];
	version->main[1] = fw_hd->main[1];
	version->main[2] = fw_hd->main[2];
}

void s2mm005_get_chip_hwversion(struct s2mm005_data *usbpd_data,
			     struct s2mm005_version *version)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm005_read_byte_flash(i2c, 0x0, (u8 *)&version->boot, 1);
	s2mm005_read_byte_flash(i2c, 0x1, (u8 *)&version->main, 3);
	s2mm005_read_byte_flash(i2c, 0x4, (u8 *)&version->ver2, 4);
}

void s2mm005_get_chip_swversion(struct s2mm005_data *usbpd_data,
			     struct s2mm005_version *version)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	int i;

	for(i=0; i < FW_CHECK_RETRY; i++) {
		s2mm005_read_byte_flash(i2c, 0x8, (u8 *)&version->boot, 1);
		if(VALID_FW_BOOT_VERSION(version->boot))
			break;
	}
	for(i=0; i < FW_CHECK_RETRY; i++) {
		s2mm005_read_byte_flash(i2c, 0x9, (u8 *)&version->main, 3);
		if(VALID_FW_MAIN_VERSION(version->main))
			break;
	}
	for (i = 0; i < FW_CHECK_RETRY; i++)
		s2mm005_read_byte_flash(i2c, 0xc, (u8 *)&version->ver2, 4);
}

int s2mm005_check_version(struct s2mm005_version *version1,
			  struct s2mm005_version *version2)
{
	if (version1->boot != version2->boot) {
		return FLASH_FW_VER_BOOT;
	}
	if (memcmp(version1->main, version2->main, 3)) {
		return  FLASH_FW_VER_MAIN;
	}

	return FLASH_FW_VER_MATCH;
}

int s2mm005_flash_fw(struct s2mm005_data *usbpd_data, unsigned int input)
{
	int ret = 0;
	u8 val = 0;

	if( usbpd_data->fw_product_id != usbpd_data->s2mm005_fw_product_id)
	{
		pr_err("FW_UPDATE fail, product number is different (%d)(%d) \n",  usbpd_data->fw_product_id,usbpd_data->s2mm005_fw_product_id);
		return 0;
	}
	
	pr_err("FW_UPDATE %d\n", input);
	switch (input) {
	case FLASH_WRITE5:
	case FLASH_WRITE6:
	case FLASH_WRITE7:	
	case FLASH_WRITE: {
		s2mm005_flash(usbpd_data, FLASH_MODE_ENTER);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_flash(usbpd_data, FLASH_ERASE);
		msleep(200);
		ret = s2mm005_flash(usbpd_data, input);
		if (ret < 0)
			return ret;	
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_flash(usbpd_data, FLASH_MODE_EXIT);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_reset(usbpd_data);
		usleep_range(10 * 1000, 10 * 1000);
		break;
	}
	case FLASH_WRITE_UMS: {
		s2mm005_read_byte_flash(usbpd_data->i2c, FLASH_STATUS_0x24, &val, 1);
		if(val != FLASH_MODE_NORMAL) {
			pr_err("Can't CCIC FW update: cause by %s\n", flashmode_to_string(val));
		}
		disable_irq(usbpd_data->irq);
		s2mm005_manual_LPM(usbpd_data, 0x7); // LP Off
		msleep(3000);
		s2mm005_flash(usbpd_data, FLASH_MODE_ENTER);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_flash(usbpd_data, FLASH_ERASE);
		msleep(200);
		ret = s2mm005_flash(usbpd_data, input);
		if (ret < 0)
			panic("infinite write fail!\n");
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_flash(usbpd_data, FLASH_MODE_EXIT);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_reset(usbpd_data);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_manual_LPM(usbpd_data, 0x6); // LP On
		enable_irq(usbpd_data->irq);
		break;
	}
	default: {
		break;
	}
	}

	return 0;
}
