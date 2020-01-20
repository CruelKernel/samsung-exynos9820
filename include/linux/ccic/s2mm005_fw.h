#ifndef _S2MM005_FW_H
#define _S2MM005_FW_H

enum {
	FLASH_MODE_ENTER,
	FLASH_ERASE,
	FLASH_WRITE,
	FLASH_WRITE3,
	FLASH_WRITE4,
	FLASH_WRITE5,
	FLASH_WRITE6,
	FLASH_WRITE7,
	FLASH_WRITE_BUILTIN,
	FLASH_WRITE_UMS,
	FLASH_MODE_EXIT,
};

enum {
	FLASH_MODE_NORMAL = 0x00,
	FLASH_MODE_FLASH = 0x80,
	FLASH_MODE_ERASE = 0x81,
};

enum {
	FLASH_FW_VER_MATCH = 0x00,
	FLASH_FW_VER_BOOT = 0x01,
	FLASH_FW_VER_MAIN = 0x02,

	/* ERROR FOR S2MM005 FLASH WRITE */
	EFLASH_VERIFY = 0x10,
	EFLASH_WRITE_SWVERSION,
	EFLASH_WRITE_SIZE,
	EFLASH_WRITE_CRC,
	EFLASH_WRITE_DONE,
};

enum {
	PRODUCT_NUM_GRACE = 0x00,
	PRODUCT_NUM_DREAM = 0x01,
	PRODUCT_NUM_GREAT = 0x0A,
};

struct s2mm005_version {
	u8 main[3];
	u8 boot;
	u8 ver2[4];
};

struct s2mm005_fw {
	u8 boot;
	u8 main[3];
	unsigned char reserve[4];
	unsigned int size;
};

const char *flashmode_to_string(u32 mode);
int s2mm005_sram_write(const struct i2c_client *i2c);
void s2mm005_write_flash(const struct i2c_client *i2c, unsigned int fAddr, unsigned int fData);
void s2mm005_verify_flash(const struct i2c_client *i2c, uint32_t fAddr, uint32_t *fRData);
int s2mm005_flash(struct s2mm005_data *usbpd_data, unsigned int input);
int s2mm005_flash_fw(struct s2mm005_data *usbpd_data, unsigned int input);
void s2mm005_get_chip_hwversion(struct s2mm005_data *usbpd_data, struct s2mm005_version *version);
void s2mm005_get_chip_swversion(struct s2mm005_data *usbpd_data, struct s2mm005_version *version);
void s2mm005_get_fw_version(int product_id, struct s2mm005_version *version, u8 boot_version, u32 hw_rev);
int s2mm005_check_version(struct s2mm005_version *version1, struct s2mm005_version *version2);
int s2mm005_flash_fw_entry(struct s2mm005_data *usbpd_data, const struct firmware *fw_entry);

#define CMD_MODE_0x10 0x10
#define CMD_HOST_0x11	0x11
#define FLASH_STATUS_0x24 0x24

#define FLASH_MODE_ENTER_0x10	0x10
#define FLASH_WRITE_0x42	0x42
#define FLASH_ERASE_0x44	0x44
#define FLASH_MODE_EXIT_0x20	0x20

#define FLASH_WRITING_BYTE_SIZE_0x4 0x4

#endif
