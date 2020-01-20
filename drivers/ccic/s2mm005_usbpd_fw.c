#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ccic/s2mm005_usbpd.h>
#include <linux/ccic/s2mm005_usbpd_phy.h>
#include <linux/ccic/s2mm005_usbpd_fw.h>
#include <linux/ccic/usbpd_sysfs.h>
#include <linux/ccic/BOOT_FLASH_FW.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT3.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT4.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT5.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT5_NODPDM.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT6.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT7.h>
#include <linux/ccic/BOOT_FLASH_FW_BOOT8.h>
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

static void s2mm005_sram_reset(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 buf;
	printk("%s\n", __func__);
	/* boot control reset OM HIGH */
	buf = 0x08;
	s2mm005_sel_write(i2c, 0x101C, &buf, SEL_WRITE_BYTE);
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
				unsigned int fAddr, unsigned int fData)
{
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
			  			uint32_t fAddr, uint32_t *fData)
{
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
		s2mm005_read_byte(i2c, REG_ADD, &R_DATA[0], 4);

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
		s2mm005_read_byte(i2c, REG_ADD, &R_DATA[0], 4);

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

static int s2mm005_flash_write_guarantee(struct s2mm005_data *usbpd_data, u16 addr, u32 data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	uint32_t RData;
	uint32_t recheck_count = 0;

	while (1) {
		recheck_count++;
		dev_info(&i2c->dev, "%s addr = 0x%04X, data = 0x%08X\n", __func__, addr, data);
		s2mm005_write_flash(i2c, addr, data);
		RData = 0;
		s2mm005_verify_flash(i2c, addr, &RData);
		if (data == RData)
			break;
		else {
			if (recheck_count == 30)
				return -EINVAL;
		}
	}

	return 0;
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
	int ret = 0;

	reg = FLASH_WRITE_0x42;
	s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
	reg = FLASH_WRITING_BYTE_SIZE_0x4;
	s2mm005_write_byte(i2c, CMD_HOST_0x11, &reg, 1);
	s2mm005_read_byte(i2c, FLASH_STATUS_0x24, &val, 1);

	pFlash_FW = (uint32_t *)(fw_data + 12);
	pFlash_FWCS = (uint32_t *)fw_data;
	fw_hd = (struct s2mm005_fw *)fw_data;
	size = fw_hd->size;
	if (fw_hd->boot < 6)
		sLopCnt = 0x1000 / 4;
	else if (fw_hd->boot == 6)
		sLopCnt = 0x8000 / 4;
	else if (fw_hd->boot == 7)
		sLopCnt = 0x7000 / 4;
	else if (fw_hd->boot >= 8)
		sLopCnt = 0x5000 / 4;

	/* Flash write */
	for (LopCnt = sLopCnt; LopCnt < (size / 4); LopCnt++) {
		fAddr = LopCnt * 4;
		fData = pFlash_FW[LopCnt];
		udelay(10);
		s2mm005_write_flash(i2c, fAddr, fData);
	}
	usleep_range(10 * 1000, 10 * 1000);
	/* Partial verify */
	while (1) {
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
				if (recheck_count == 1000)
					return -EFLASH_VERIFY;
				break;
			}
		}
		if (fError)
			break;
	}
	pr_err("%s verify success!! recheck count : %d\n", __func__, recheck_count);

	if (LopCnt >= (size / 4)) {
		if (fw_hd->boot >= 6) {
			/* SW version from header */
			ret = s2mm005_flash_write_guarantee(usbpd_data, 0xEFF0, pFlash_FWCS[0]);
			if (ret < 0)
				return ret;
			/* Size from header */
			ret = s2mm005_flash_write_guarantee(usbpd_data, 0xEFF4, pFlash_FWCS[2]);
			if (ret < 0)
				return ret;

			/* CRC Check sum */
			ret = s2mm005_flash_write_guarantee(usbpd_data, 0xEFF8,
						pFlash_FWCS[((size + 16) / 4) - 1]);
			if (ret < 0)
				return ret;
		}

		/* flash done */
		ret = s2mm005_flash_write_guarantee(usbpd_data, 0xEFFC, 0x1);
		if (ret < 0)
			return ret;

		pr_err("%s flash write succesfully done!!\n", __func__);
	}

	return 0;
}

void s2mm005_flash_ready(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 buf = 0x01;

	/* FLASH_READY */
	s2mm005_sel_write(i2c, 0x5030, &buf, SEL_WRITE_BYTE);
}

static void s2mm005_flash_enter(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg;
	int retry = 0;
	int irq_gpio_status;

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
		if (!irq_gpio_status) {
			s2mm005_int_clear(usbpd_data);
			usleep_range(10 * 1000, 10 * 1000);
		}
		s2mm005_read_byte(i2c, FLASH_STATUS_0x24, &val, 1);
		pr_err("flash mode : %s retry %d\n", flashmode_to_string(val), retry);
		usleep_range(50 * 1000, 50 * 1000);

		if (val != FLASH_MODE_FLASH) {
			retry++;
			if (retry == 10) {
				/* RESET */
				s2mm005_reset(usbpd_data);
				msleep(3000);
				/* FLASH_READY */
				s2mm005_flash_ready(usbpd_data);
			} else if (retry == 20)
				panic("Flash mode change fail!\n");
		}
	} while (val != FLASH_MODE_FLASH);
}

static void s2mm005_flash_erase(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg;

	reg = FLASH_ERASE_0x44;
	usleep_range(10 * 1000, 10 * 1000);
	s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
	msleep(200);
	s2mm005_read_byte(i2c, FLASH_STATUS_0x24, &val, 1);
	pr_err("flash mode : %s\n", flashmode_to_string(val));
}

static int s2mm005_flash_write_ums(struct s2mm005_data *usbpd_data)
{
	int ret = 0;
	struct s2mm005_fw *fw_hd;
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(CCIC_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		pr_err("%s: failed to open %s.\n", __func__,
					CCIC_DEFAULT_UMS_FW);
		ret = -ENOENT;
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

		if (nread != fw_size)
			pr_err("%s: failed to read firmware file, nread %ld Bytes\n",
				      __func__, nread);
		else {
			fw_hd = (struct s2mm005_fw *)fw_data;
			pr_err("%02X %02X %02X %02X size:%05d\n", fw_hd->boot, fw_hd->main[0], fw_hd->main[1], fw_hd->main[2], fw_hd->size);
			/* TODO : DISABLE IRQ */
			/* TODO : FW UPDATE */
			ret = s2mm005_flash_write(usbpd_data, (unsigned char *)fw_data);
			/* TODO : ENABLE IRQ */
		}
		kfree(fw_data);
	}

	filp_close(fp, NULL);
	set_fs(old_fs);

	return ret;
}

static int s2mm005_flash_sram(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg;
	int ret = 0;
	uint32_t *pFlash_FW;
	uint32_t LopCnt, fAddr, fData, fRData;
	struct s2mm005_fw *fw_hd;

	fw_hd = (struct s2mm005_fw *)&BOOT_FLASH_FW_BOOT4;
	reg = FLASH_WRITE_0x42;
	s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
	reg = FLASH_WRITING_BYTE_SIZE_0x4;
	s2mm005_write_byte(i2c, CMD_HOST_0x11, &reg, 1);
	s2mm005_read_byte(i2c, FLASH_STATUS_0x24, &val, 1);

	pFlash_FW = (uint32_t *)&BOOT_FLASH_FW_BOOT4[0];
	fAddr = 0x00000000;
	for ((LopCnt = 0); LopCnt < (fw_hd->size/4); LopCnt++) {
		fAddr = LopCnt*4;
		fData = pFlash_FW[LopCnt];
		s2mm005_write_flash(i2c, fAddr, fData);
		s2mm005_verify_flash(i2c, fAddr, &fRData);
		if (fData != fRData) {
			pr_err("Verify Error Address = 0x%08X    WData = 0x%08X    VData = 0x%08X\n", fAddr, fData, fRData);
			return -EFLASH_VERIFY;
		}
	}
	if (LopCnt >= (fw_hd->size/4)) {
		fAddr = 0xeFFC;
		fData = 0x1;
		s2mm005_write_flash(i2c, fAddr, fData);
		s2mm005_verify_flash(i2c, fAddr, &fRData);
		if (fData != fRData) {
			pr_err("Verify Error Address = 0x%08X    WData = 0x%08X    VData = 0x%08X\n", fAddr, fData, fRData);
			return -EFLASH_VERIFY;
		}
	}

	return ret;
}

int s2mm005_flash(struct s2mm005_data *usbpd_data, unsigned int input)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val, reg;
	int ret = 0;

	switch (input) {
	case FLASH_MODE_ENTER: /* enter flash mode */
		s2mm005_flash_enter(usbpd_data);
		break;
	case FLASH_ERASE: /* erase flash */
		s2mm005_flash_erase(usbpd_data);
		break;
	case FLASH_WRITE: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW[0]);
		break;
	case FLASH_WRITE3: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT3[0]);
		break;
	case FLASH_WRITE4: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT4[0]);
		break;
	case FLASH_WRITE5: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT5[0]);
		break;
	case FLASH_WRITE6: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT6[0]);
		break;
	case FLASH_WRITE7: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT7[0]);
		break;
	case FLASH_WRITE8: /* write flash & verify */
		ret = s2mm005_flash_write(usbpd_data, (unsigned char *)&BOOT_FLASH_FW_BOOT8[0]);
		break;
	case FLASH_WRITE_UMS:
		ret = s2mm005_flash_write_ums(usbpd_data);
		if (ret < 0)
			goto done;
		break;
	case FLASH_SRAM: /* write flash & verify */
		ret = s2mm005_flash_sram(usbpd_data);
		if (ret < 0)
			return ret;
		break;
	case FLASH_MODE_EXIT: /* exit flash mode */
		reg = FLASH_MODE_EXIT_0x20;
		s2mm005_write_byte(i2c, CMD_MODE_0x10, &reg, 1);
		usleep_range(15 * 1000, 15 * 1000);
		s2mm005_read_byte(i2c, FLASH_STATUS_0x24, &val, 1);
		pr_err("flash mode : %s\n", flashmode_to_string(val));
		break;
	default:
		pr_err("Flash value does not matched menu\n");

	}
done:
	return ret;
}

void s2mm005_get_fw_version(struct s2mm005_version *version, u8 boot_version, u32 hw_rev)
{
	struct s2mm005_fw *fw_hd;
	switch (boot_version) {
	case 5:
		fw_hd = (struct s2mm005_fw *) BOOT_FLASH_FW_BOOT5;
		break;
	case 6:
		fw_hd = (struct s2mm005_fw *) BOOT_FLASH_FW_BOOT6;
		break;
	case 7:
		fw_hd = (struct s2mm005_fw *) BOOT_FLASH_FW_BOOT7;
		break;
	case 8:
		fw_hd = (struct s2mm005_fw *) BOOT_FLASH_FW_BOOT8;
		break;
	default:
		fw_hd = (struct s2mm005_fw *) BOOT_FLASH_FW_BOOT8;
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

	s2mm005_read_byte(i2c, 0x0, (u8 *)&version->boot, 1);
	s2mm005_read_byte(i2c, 0x1, (u8 *)&version->main, 3);
}

void s2mm005_get_chip_swversion(struct s2mm005_data *usbpd_data,
			     struct s2mm005_version *version)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	int i;

	for (i = 0; i < FW_CHECK_RETRY; i++) {
		s2mm005_read_byte(i2c, 0x8, (u8 *)&version->boot, 1);
		if (VALID_FW_BOOT_VERSION(version->boot))
			break;
	}
	for (i = 0; i < FW_CHECK_RETRY; i++) {
		s2mm005_read_byte(i2c, 0x9, (u8 *)&version->main, 3);
		if (VALID_FW_MAIN_VERSION(version->main))
			break;
	}
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

static int s2mm005_flash_fw_process(struct s2mm005_data *usbpd_data, unsigned int input)
{
	int ret = 0;

	s2mm005_flash(usbpd_data, FLASH_MODE_ENTER);
	usleep_range(10 * 1000, 10 * 1000);
	s2mm005_flash(usbpd_data, FLASH_ERASE);
	if (input != FLASH_SRAM)
		msleep(200);
	ret = s2mm005_flash(usbpd_data, input);
	if (ret < 0)
		return ret;
	usleep_range(10 * 1000, 10 * 1000);
	s2mm005_flash(usbpd_data, FLASH_MODE_EXIT);

	return ret;
}

int s2mm005_flash_fw(struct s2mm005_data *usbpd_data, unsigned int input)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	int ret = 0;
	u8 val = 0;
	u8 check[4];
	struct device *i2c_dev = i2c->dev.parent->parent;
	struct pinctrl *i2c_pinctrl;
#if 0
	if (usbpd_data->fw_product_num != PRODUCT_NUM) {
		pr_err("FW_UPDATE fail, product number is different (%d)(%d) \n",
					   usbpd_data->fw_product_num, PRODUCT_NUM);
		return 0;
	}
#endif
	pr_err("FW_UPDATE %d\n", input);
	switch (input) {
	case FLASH_WRITE ... FLASH_WRITE8:
		ret = s2mm005_flash_fw_process(usbpd_data, input);
		if (ret < 0)
			return ret;
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_reset(usbpd_data);
		usleep_range(10 * 1000, 10 * 1000);
		break;
	case FLASH_WRITE_UMS:
		s2mm005_read_byte(usbpd_data->i2c, FLASH_STATUS_0x24, &val, 1);
		if (val != FLASH_MODE_NORMAL)
			pr_err("Can't CCIC FW update: cause by %s\n", flashmode_to_string(val));
		disable_irq(usbpd_data->irq);
		s2mm005_manual_LPM(usbpd_data, 0x7);
		msleep(3000);
		ret = s2mm005_flash_fw_process(usbpd_data, input);
		if (ret < 0)
			return ret;
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_reset(usbpd_data);
		usleep_range(10 * 1000, 10 * 1000);
		s2mm005_manual_LPM(usbpd_data, 0x6);
		enable_irq(usbpd_data->irq);
		break;
	case FLASH_SRAM:
		s2mm005_system_reset(usbpd_data);
		s2mm005_reset_enable(usbpd_data);
		s2mm005_sram_reset(usbpd_data);
		i2c_pinctrl = devm_pinctrl_get_select(i2c_dev, "om_high");
		if (IS_ERR(i2c_pinctrl))
			pr_err("could not set om high pins\n");
		s2mm005_hard_reset(usbpd_data);
		s2mm005_sram_write(i2c);
		usleep_range(1 * 1000, 1 * 1000);

		s2mm005_sel_read(i2c, 0x2000, check, SEL_READ_LONG);

		pr_err("%s sram write size:%2x,%2x,%2x,%2x\n", __func__, check[3], check[2], check[1], check[0]);

		ret = s2mm005_read_byte(i2c, 0xC, &check[0], 4);
		pr_err("%s sram check :%2x,%2x,%2x,%2x\n", __func__, check[3], check[2], check[1], check[0]);

		ret = s2mm005_flash_fw_process(usbpd_data, input);
		if (ret < 0)
			return ret;

		i2c_pinctrl = devm_pinctrl_get_select(i2c_dev, "om_input");
		if (IS_ERR(i2c_pinctrl))
			pr_err("could not set reset pins\n");
		s2mm005_hard_reset(usbpd_data);
		usleep_range(10 * 1000, 10 * 1000);
		break;
	default:
		break;
	}

	return 0;
}

int s2mm005_fw_ver_check(void *data)
{
	struct s2mm005_data *usbpd_data = data;
	struct s2mm005_version chip_swver, hwver;

	if ((usbpd_data->firm_ver[1] == 0xFF && usbpd_data->firm_ver[2] == 0xFF)
		|| (usbpd_data->firm_ver[1] == 0x00 && usbpd_data->firm_ver[2] == 0x00)) {
		s2mm005_get_chip_hwversion(usbpd_data, &hwver);
		pr_err("%s CHIP HWversion %2x %2x %2x %2x\n", __func__,
			hwver.main[2], hwver.main[1], hwver.main[0], hwver.boot);

		s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
		pr_err("%s CHIP SWversion %2x %2x %2x %2x\n", __func__,
		       chip_swver.main[2], chip_swver.main[1], chip_swver.main[0], chip_swver.boot);

	if ((chip_swver.main[0] == 0xFF && chip_swver.main[1] == 0xFF)
		|| (chip_swver.main[0] == 0x00 && chip_swver.main[1] == 0x00)) {
			pr_err("%s Invalid FW version\n", __func__);
			return CCIC_FW_VERSION_INVALID;
		}

		store_ccic_version(&hwver.main[0], &chip_swver.main[0], &chip_swver.boot);
		usbpd_data->firm_ver[0] = chip_swver.main[2];
		usbpd_data->firm_ver[1] = chip_swver.main[1];
		usbpd_data->firm_ver[2] = chip_swver.main[0];
		usbpd_data->firm_ver[3] = chip_swver.boot;
	}
	return 0;
}

int s2mm005_usbpd_firmware_check(struct s2mm005_data *usbpd_data)
{
	struct s2mm005_version chip_swver, fw_swver, hwver;
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 W_DATA[4];
	u8 R_DATA[4];
	u8 temp = 0, ftrim = 0, chip_ver = 0, latest_ver = 0;

	s2mm005_get_chip_hwversion(usbpd_data, &hwver);
	dev_info(usbpd_data->dev, "%s CHIP HWversion %2x %2x %2x %2x\n", __func__,
	       hwver.main[2], hwver.main[1], hwver.main[0], hwver.boot);
	if (hwver.boot <= 2) {
		s2mm005_sel_read(i2c, 0x1104, R_DATA, SEL_READ_LONG);
		dev_info(usbpd_data->dev, "%s ftrim:%02X %02X %02X %02X\n",
			__func__, R_DATA[0], R_DATA[1], R_DATA[2], R_DATA[3]);

		ftrim = ((R_DATA[1] & 0xF8) >> 3) - 2;
		temp = R_DATA[1] & 0x7;
		R_DATA[1] = (ftrim << 3) + temp;
		dev_info(usbpd_data->dev, "%s ftrim:%02X %02X %02X %02X\n",
			__func__, R_DATA[0], R_DATA[1], R_DATA[2], R_DATA[3]);

		W_DATA[0] = R_DATA[0]; W_DATA[1] = R_DATA[1]; W_DATA[2] = R_DATA[2]; W_DATA[3] = R_DATA[3];
		s2mm005_sel_write(i2c, 0x1104, W_DATA, SEL_WRITE_LONG);

		s2mm005_sel_read(i2c, 0x1104, R_DATA, SEL_READ_LONG);
		dev_info(usbpd_data->dev, "%s ftrim:%02X %02X %02X %02X\n",
			__func__, R_DATA[0], R_DATA[1], R_DATA[2], R_DATA[3]);

	}

	s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
	dev_info(usbpd_data->dev, "%s CHIP SWversion BEFORE : %2x %2x %2x %2x\n", __func__,
	       chip_swver.main[2], chip_swver.main[1], chip_swver.main[0], chip_swver.boot);

	chip_ver = chip_swver.main[0];

	s2mm005_get_fw_version(&fw_swver, chip_swver.boot, usbpd_data->hw_rev);
	dev_info(usbpd_data->dev, "%s LATEST SWversion : %2x,%2x,%2x,%2x\n", __func__,
		fw_swver.main[2], fw_swver.main[1], fw_swver.main[0], fw_swver.boot);

	latest_ver = fw_swver.main[0];

	dev_info(usbpd_data->dev, "%s: FW UPDATE boot : %01d hw_rev : %02d\n", __func__, chip_swver.boot, usbpd_data->hw_rev);

	usbpd_data->fw_product_num = fw_swver.main[2];

	if (chip_swver.boot == 0x7) {
#ifdef CONFIG_SEC_FACTORY
		if ((chip_ver != latest_ver)
			|| (chip_swver.main[1] != fw_swver.main[1])) {
			if (s2mm005_flash_fw(usbpd_data, chip_swver.boot) < 0) {
				dev_info(usbpd_data->dev, "%s: s2mm005_flash_fw 1st fail, try again \n", __func__);
				if (s2mm005_flash_fw(usbpd_data, chip_swver.boot) < 0) {
					dev_info(usbpd_data->dev, "%s: s2mm005_flash_fw 2st fail, panic \n", __func__);
					panic("infinite write fail!\n");
				}
			}
		}
#else
		if ((chip_ver < latest_ver)
			|| ((chip_ver == latest_ver) && (chip_swver.main[1] < fw_swver.main[1])))
			s2mm005_flash_fw(usbpd_data, chip_swver.boot);
		else if ((((chip_swver.main[2] == 0xff) && (chip_swver.main[1] == 0xa5)) || chip_swver.main[2] == 0x00) &&
			fw_swver.main[2] != 0x0)
			s2mm005_flash_fw(usbpd_data, chip_swver.boot);
#endif

		s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
		dev_info(usbpd_data->dev, "%s CHIP SWversion AFTER : %2x %2x %2x %2x\n", __func__,
		       chip_swver.main[2], chip_swver.main[1], chip_swver.main[0], chip_swver.boot);
	}

	if (chip_swver.boot == 0x8) {
#ifdef CONFIG_SEC_FACTORY
		if ((chip_ver != latest_ver)
			|| (chip_swver.main[1] != fw_swver.main[1])) {
			if (s2mm005_flash_fw(usbpd_data, chip_swver.boot) < 0) {
				dev_info(usbpd_data->dev, "%s: s2mm005_flash_fw 1st fail, try again \n", __func__);
				if (s2mm005_flash_fw(usbpd_data, chip_swver.boot) < 0) {
					dev_info(usbpd_data->dev, "%s: s2mm005_flash_fw 2st fail, panic \n", __func__);
					panic("infinite write fail!\n");
				}
			}
		}
#else
		if ((chip_ver != latest_ver)
			|| ((chip_ver == latest_ver) && (chip_swver.main[1] < fw_swver.main[1])))
			s2mm005_flash_fw(usbpd_data, chip_swver.boot);
		else if ((((chip_swver.main[2] == 0xff) && (chip_swver.main[1] == 0xa5))   || chip_swver.main[2] == 0x00) &&
			fw_swver.main[2] != 0x0)
			s2mm005_flash_fw(usbpd_data, chip_swver.boot);
#endif

		s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
		dev_info(usbpd_data->dev, "%s CHIP SWversion AFTER : %2x %2x %2x %2x\n", __func__,
		       chip_swver.main[2], chip_swver.main[1], chip_swver.main[0], chip_swver.boot);
	}

	usbpd_data->firm_ver[0] = chip_swver.main[2];
	usbpd_data->firm_ver[1] = chip_swver.main[1];
	usbpd_data->firm_ver[2] = chip_swver.main[0];

	return 1;
}
